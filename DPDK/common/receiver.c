/*
 * receiver.c
 *
 *  Created on: Apr 14, 2016
 *      Author: stephan
 */

#include "receiver.h"
#include "common.h"
#include "config.h"
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

int receive_pkt(__attribute__((unused)) void *args) {

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

    gettimeofday(&timeval, NULL);
    recdTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;

	mkdir("Logging", 0777);

    sprintf(fileLog, "Logging/log-%ld", recdTime);
    fLog = fopen(fileLog, "w+");

    while(RCV_PKT && (sampleIndex < NUM_SAMPLES)) {

    	TRIGGER_RECEIVER = 0;

        ret = rte_eth_rx_burst(RX_PORT_NO, RX_QUEUE_ID, recd_bufs, RX_RING_SIZE);

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

					if (DEBUG)
						printf("DPDK RECEIVER: RECEIVING Packet nr. %d with latency of %ld\n", seqNo[sampleIndex], latency[sampleIndex]);

                    sampleIndex++;
                }

                rte_pktmbuf_free(recd_bufs[index]);
            }
        }
    }

    // If logging is needed, write into logfile
    for (i = 0; i < sampleIndex; i++)
        fprintf(fLog, "%ld %ld %ld\n", seqNo[i], sentTimes[i], latency[i]);
    fclose(fLog);

    SND_PKT = 0;
    PKT_TRIGGER = 0;
	printf("DPDK RECEIVER: RETURNING ...\n");
    return 0;
}

