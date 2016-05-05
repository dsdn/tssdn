/*
 * sender.h
 *
 *  Created on: Apr 14, 2016
 *      Author: stephan
 */

#ifndef _NCS_SIM_DPDK_SENDER_H_
#define _NCS_SIM_DPDK_SENDER_H_

void sig_int(int signo);
void sig_timer(int signo);
void signal_handler(int sig);

int send_pkt(__attribute__((unused)) void *args);


#endif /* _NCS_SIM_DPDK_SENDER_H_ */
