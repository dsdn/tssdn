/*
 * sender.c
 *
 *  Created on: Apr 14, 2016
 *      Author: stephan
 */

#include "sender.h"
#include "common.h"
#include "config.h"

int send_pkt(__attribute__((unused)) void *args) {


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
        fprintf(stderr, "DPDK SENDER: Error creating timer");
        exit(0);
    }

    gettimeofday(&timeval, NULL);
    its.it_value.tv_sec = timeval.tv_sec + 1;
    its.it_value.tv_nsec = BASE_PERIOD_NS + SLOT_OFFSET_NS - PKT_CREATION_TIME_NS;
    its.it_interval.tv_sec = SEND_PERIOD_SEC;
    its.it_interval.tv_nsec = SEND_PERIOD_NSEC;
    if (timer_settime(timerid, TIMER_ABSTIME, &its, NULL) < 0) {

        printf("DPDK SENDER: Error setting timer - %s.\n", strerror(errno));
    }

    PKT_TRIGGER = 1;

    // While loop for running the sending process
    while(SND_PKT) {

    	// Loop for waiting for the trigger set by the interrupt
        while(PKT_TRIGGER & PRE_WAIT) {

        }

        // Sending as many packets as defined in TX_BURST_SIZE
        for (index = 0; index < TX_BURST_SIZE; index++) {

            send_bufs[index] = rte_pktmbuf_alloc(args);
            if(send_bufs[index] == NULL) {
                printf("DPDK SENDER: Error assigning mbuf pkt\n");
            }

            // Set the packets headers
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

            // If VLAN is defined, add respective fields to the packet
            if (VLAN_PKT)
            {
                send_bufs[index]->l2_len += sizeof(struct vlan_hdr);
                send_bufs[index]->vlan_tci = VLAN_PRIO << 13 | 0x00 | VLAN_TAG;
                rte_vlan_insert(&send_bufs[index]);
            }

            // Wait for ??????
            while(PKT_TRIGGER & !PRE_WAIT) {

            }


            // Add to the payload of the packet
            //if (gettimeofday(&timeval, NULL) >= 0) {

                //sentTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
                //memcpy((char*)(payload_ptr), (void*)y, 3 * sizeof(double));

           //printf("SENDER PAYLOAD: %p\n",payload_ptr);

           memcpy((char*)(payload_ptr), (void*)&seqNo, sizeof(unsigned long int));
           seqNo++;

            //} else {

            //    fprintf(stderr, "DPDK SENDER: Error acquiring timestamp - %s\n", strerror(errno));
            //   	exit(0);
            //}

        }

        // Send the packet via dpdk
        ret = rte_eth_tx_burst(TX_PORT_NO, TX_QUEUE_ID, send_bufs, TX_BURST_SIZE);
        if (unlikely(ret < TX_BURST_SIZE)) {
            for (index = ret; index < TX_BURST_SIZE; index++)
                rte_pktmbuf_free(send_bufs[index]);
        }

        if (DEBUG) {
            printf("%ld\n", sentTime);
        }

        // Signal receiver that a packet has been sent
		TRIGGER_RECEIVER = 1;
		printf("DPDK SENDER: SENDING Packet nr. %d\n", seqNo-1);
        PKT_TRIGGER = 1;
    }

	printf("DPDK SENDER: RETURNING ...\n");

	return 0;
}
