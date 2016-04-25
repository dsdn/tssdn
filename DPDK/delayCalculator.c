/*
Program to calculate the propagation delay of network packets 
using DPDK API's

Naresh Nayak
29.07.2015
*/

#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>

#define DEBUG 0

#define NUM_MBUFS 8000
#define MBUF_CACHE_SIZE 256
#define TX_RING_SIZE 512
#define TX_BURST_SIZE 1
#define RX_RING_SIZE 512

// Period for sending packets
#define SEND_PERIOD_USEC 100
#define SEND_PERIOD_NSEC (SEND_PERIOD_USEC * pow(10, 3))
#define SEND_PERIOD_SEC 0
#define BASE_PERIOD_NS pow(10,5)
#define PKT_CREATION_TIME_NS (1 * pow(10, 3))
#define SLOT_OFFSET_NS (0 * pow(10, 3))

#define PAYLOAD_SIZE 40

// Number of samples to capture
#define NUM_SAMPLES 100000
#define VLAN_PKT   1
#define VLAN_TAG   10
#define VLAN_PRIO  7
#define DSCP       0
#define PRE_WAIT   1

// Operational parameters
volatile int recvPkt = 1;
volatile int sendPkt = 1;
volatile int pktTrigger = 1;

// Configuration parameters
const int txPortNo = 0;
const int rxPortNo = 1;
const int txQueues = 1, rxQueues = 1;
const int txQueueId = 0;
const int rxQueueId = 0;
struct ether_addr src_address = { .addr_bytes = {00,11,22,33,44,55} };
struct ether_addr dst_address = { .addr_bytes = {66,77,88,99,0xAA,0xBB} };
static struct rte_eth_conf port_conf = {
    .rxmode = {
        .mq_mode        = ETH_MQ_RX_RSS,
        .max_rx_pkt_len = ETHER_MAX_LEN,
        .split_hdr_size = 0,
        .header_split   = 0, /**< Header Split disabled */
        .hw_ip_checksum = 1, /**< IP checksum offload enabled */
        .hw_vlan_filter = 0, /**< VLAN filtering disabled */
        .jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
        .hw_strip_crc   = 0, /**< CRC stripped by hardware */
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = NULL,
            .rss_hf = ETH_RSS_PROTO_MASK,
        },
    },
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    }
};


// Signal handlers
static void sig_int(int signo)
{
    recvPkt = 0;
    sendPkt = 0;
    pktTrigger = 0;
    fprintf(stderr, "Exiting..\n");
}

static void sig_timer(int signo)
{
    pktTrigger = 0;
    if (DEBUG) fprintf(stderr, "Alarm..\n");
}

/* Local Functions */
static void signal_handler(int sig)
{
    if (sig == SIGINT) sig_int(sig);
    else if (sig == SIGRTMIN) sig_timer(sig);
}

// Function to send a packet every one second
static int send_pkt(__attribute__((unused)) void *args) {

    struct ether_hdr *eth_header;
    struct ipv4_hdr *ipv4_header;
    struct udp_hdr *udp_header;
    unsigned char *payload_ptr;
    struct rte_mbuf *send_bufs[1000];
    unsigned long int sentTime, seqNo = 1;
    int ret, index;   
    struct timeval timeval;
    struct itimerspec its; 
    timer_t timerid;
    struct sigevent sev;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) < 0) {
        fprintf(stderr, "Error creating timer");
        exit(0);
    }

    gettimeofday(&timeval, NULL);
    its.it_value.tv_sec = timeval.tv_sec + 1;
    its.it_value.tv_nsec = BASE_PERIOD_NS + SLOT_OFFSET_NS - PKT_CREATION_TIME_NS;
    its.it_interval.tv_sec = SEND_PERIOD_SEC;
    its.it_interval.tv_nsec = SEND_PERIOD_NSEC;
    if (timer_settime(timerid, TIMER_ABSTIME, &its, NULL) < 0) {

        printf("Error setting timer - %s.\n", strerror(errno));
    }

    pktTrigger = 1;

    while(sendPkt) {

        while(pktTrigger & PRE_WAIT) {

        }

        for (index = 0; index < TX_BURST_SIZE; index++) {

            send_bufs[index] = rte_pktmbuf_alloc(args);
            if(send_bufs[index] == NULL) {
                printf("Error assigning mbuf pkt\n");
            }

            eth_header = rte_pktmbuf_mtod(send_bufs[index], struct ether_hdr *);
            ipv4_header = (struct ipv4_hdr *)(rte_pktmbuf_mtod(send_bufs[index], unsigned char *) + sizeof(struct ether_hdr));
            udp_header = (struct udp_hdr *)(rte_pktmbuf_mtod(send_bufs[index], unsigned char *) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
            payload_ptr = (rte_pktmbuf_mtod(send_bufs[index], unsigned char *) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));

            ether_addr_copy(&src_address, &eth_header->s_addr);
            ether_addr_copy(&dst_address, &eth_header->d_addr);
            eth_header->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

            ipv4_header->version_ihl = 0x45; // Version=4, IHL=5
            ipv4_header->type_of_service = DSCP << 2; // 46 is DSCP Expedited Forwarding
            ipv4_header->packet_id = 0;
            ipv4_header->fragment_offset = 0;
            ipv4_header->time_to_live = 15;
            ipv4_header->next_proto_id = 17;
            ipv4_header->src_addr = rte_cpu_to_be_32(IPv4(10,100,1,2));
            ipv4_header->dst_addr = rte_cpu_to_be_32(IPv4(10,100,1,3));

            udp_header->src_port = rte_cpu_to_be_16(9000);
            udp_header->dst_port = rte_cpu_to_be_16(9000);
            udp_header->dgram_len = rte_cpu_to_be_16(sizeof(struct udp_hdr)+ PAYLOAD_SIZE);
            udp_header->dgram_cksum = 0;
            udp_header->dgram_cksum = rte_ipv4_phdr_cksum(ipv4_header, send_bufs[index]->ol_flags);

            rte_pktmbuf_append(send_bufs[index], ETHER_HDR_LEN + sizeof(struct vlan_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + PAYLOAD_SIZE);
            ipv4_header->total_length = rte_cpu_to_be_16(sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + PAYLOAD_SIZE);
            ipv4_header->hdr_checksum = 0;
            send_bufs[index]->ol_flags = 0;
            send_bufs[index]->ol_flags |= PKT_TX_IP_CKSUM | PKT_TX_UDP_CKSUM | PKT_TX_IPV4;
            send_bufs[index]->l2_len = sizeof(struct ether_hdr);
            send_bufs[index]->l3_len = sizeof(struct ipv4_hdr);

            if (VLAN_PKT)
            {
                send_bufs[index]->l2_len += sizeof(struct vlan_hdr);
                send_bufs[index]->vlan_tci = VLAN_PRIO << 13 | 0x00 | VLAN_TAG;
                rte_vlan_insert(&send_bufs[index]);
            }

            while(pktTrigger & !PRE_WAIT) {

            }

            // Send the current time
            if (gettimeofday(&timeval, NULL) >= 0) {

                sentTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
                memcpy((char*)(payload_ptr), (void*)&sentTime, sizeof(unsigned long int));
                memcpy((char*)(payload_ptr + sizeof(unsigned long int)), (void*)&seqNo, sizeof(unsigned long int));
                seqNo++;
            } else {

                fprintf(stderr, "Error acquiring timestamp - %s\n", strerror(errno));
               	exit(0);
            }

        }

        ret = rte_eth_tx_burst(txPortNo, txQueueId, send_bufs, TX_BURST_SIZE);
        if (unlikely(ret < TX_BURST_SIZE)) {
            for (index = ret; index < TX_BURST_SIZE; index++)
                rte_pktmbuf_free(send_bufs[index]);
        }

        if (DEBUG) {
            printf("%ld\n", sentTime);
        }

        pktTrigger = 1;
    }

    return 0;
}

// Function to receive
static int receive_pkt(__attribute__((unused)) void *args) {

    unsigned long int ret, index, sampleIndex = 0;
    unsigned long int i;
    char fileLog[32];
    struct rte_mbuf *recd_bufs[RX_RING_SIZE];
    unsigned char *payload_ptr;
    FILE *fLog;
    unsigned long int recdTime, sentTime;
    unsigned long int sentTimes[NUM_SAMPLES];
    unsigned long int latency[NUM_SAMPLES];
    unsigned long int seqNo[NUM_SAMPLES];
    struct timeval timeval;

    // Open a file to log timings
    gettimeofday(&timeval, NULL);
    recdTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
    sprintf(fileLog, "log-%ld", recdTime);
    fLog = fopen(fileLog, "w+");

    // Allocate packet on the current socket
    while(recvPkt && (sampleIndex < NUM_SAMPLES)) {
        ret = rte_eth_rx_burst(rxPortNo, rxQueueId, recd_bufs, RX_RING_SIZE);

        if(ret > 0) {

            for (index = 0; index < ret; index++) {

                payload_ptr = (rte_pktmbuf_mtod(recd_bufs[index], unsigned char *) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
                if (VLAN_PKT) {
                    payload_ptr += sizeof(struct vlan_hdr);   
                }
                if (gettimeofday(&timeval, NULL) >= 0) {

                    recdTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
                    memcpy(&sentTime, payload_ptr, sizeof(unsigned long int));
                    memcpy(&seqNo[sampleIndex], payload_ptr + sizeof(unsigned long int), sizeof(unsigned long int));
                    sentTimes[sampleIndex] = sentTime;
                    latency[sampleIndex] = recdTime - sentTime;
                    sampleIndex++;
                }

                rte_pktmbuf_free(recd_bufs[index]);
            }
        }
    }

    for (i = 0; i < sampleIndex; i++)
        fprintf(fLog, "%ld %ld %ld\n", seqNo[i], sentTimes[i], latency[i]);
    fclose(fLog);

    sendPkt = 0;
    pktTrigger = 0;
    return 0;
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[]) {

    int ret;
    struct rte_mempool *mbuf_pool;

    struct sigaction newSigAction;
    memset((void *)&newSigAction, 0, sizeof(struct sigaction));
    newSigAction.sa_handler = &signal_handler;
    newSigAction.sa_flags = SA_NODEFER;

    /* Initialize the Environment Abstraction Layer (EAL). */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    /* Creates a new mempool in memory to hold the msend_bufs. */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // Configure port for sending
    ret = rte_eth_dev_configure(txPortNo, 0, txQueues, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Port could not be configured\n");

    // Configure port for receiving
    ret = rte_eth_dev_configure(rxPortNo, rxQueues, 0, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Port could not be configured.\n");


    // Setup a transmit queue for tx port 
    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(txPortNo, &dev_info);
    struct rte_eth_txconf *txconf  = &dev_info.default_txconf;
    txconf->txq_flags = 0;
    ret = rte_eth_tx_queue_setup(txPortNo, txQueueId, TX_RING_SIZE, rte_eth_dev_socket_id(txPortNo), txconf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not set-up the transmit queue for port\n");

    // Setup a receive queue for port 
    ret = rte_eth_rx_queue_setup(rxPortNo, rxQueueId, RX_RING_SIZE, rte_eth_dev_socket_id(rxPortNo), NULL, mbuf_pool);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not set-up the receive queue for port.\n");

    // Start the port
    ret = rte_eth_dev_start(txPortNo);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not start port\n");
    ret = rte_eth_dev_start(rxPortNo);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not start port\n");

    /* Enable RX in promiscuous mode for the Ethernet device. */
    rte_eth_promiscuous_enable(txPortNo);
    rte_eth_promiscuous_enable(rxPortNo);

    // Attach signal handlers
    if (sigaction(SIGINT, &newSigAction, NULL) < 0) {
        fprintf(stderr, "Error attaching signal handler.\n");
        exit(0);
    }
    if (sigaction(SIGALRM, &newSigAction, NULL) < 0) {
        fprintf(stderr, "Error attaching signal handler.\n");
        exit(0);
    }
    if (sigaction(SIGRTMIN, &newSigAction, NULL) < 0) {
        fprintf(stderr, "Error attaching signal handler.\n");
        exit(0);
    }

    sleep(2);

    /*Start sending on the port*/
    ret = rte_eal_remote_launch(send_pkt, mbuf_pool, 3);
    ret = rte_eal_remote_launch(receive_pkt, mbuf_pool, 2);

    rte_eal_mp_wait_lcore();

    return 0;
}
