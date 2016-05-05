/*
 * common.h
 *
 *  Created on: Apr 14, 2016
 *      Author: stephan
 */

#ifndef _NCS_SIM_DPDK_COMMON_H_
#define _NCS_SIM_DPDK_COMMON_H_

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

#define NUM_MBUFS 8000
#define MBUF_CACHE_SIZE 256
#define TX_RING_SIZE 512
#define TX_BURST_SIZE 1
#define RX_RING_SIZE 512

#define BASE_PERIOD_NS pow(10,5)
#define PKT_CREATION_TIME_NS (1 * pow(10, 3))
#define SLOT_OFFSET_NS (0 * pow(10, 3))

#define PAYLOAD_SIZE 40
#define PRE_WAIT   1

typedef struct rte_mempool* mempoolptr;

// Configuration parameters
extern const int TX_QUEUES;
extern const int TX_QUEUE_ID;
extern const int RX_QUEUES;
extern const int RX_QUEUE_ID;

// Operational parameters
extern volatile int RCV_PKT;
extern volatile int SND_PKT;
extern volatile int PKT_TRIGGER;
extern volatile int TRIGGER_RECEIVER;

extern struct ether_addr src_address;
extern struct ether_addr dst_address;
extern struct rte_eth_conf port_conf;

void setup_sender(mempoolptr);
void setup_receiver(mempoolptr);

// Signal handlers
void sig_int(int signo);
void sig_timer(int signo);
void signal_handler(int sig);


#endif // _NCS_SIM_DPDK_COMMON_H_
