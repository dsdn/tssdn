/*
 * simulation.c
 *
 *  Created on: Apr 28, 2016
 *      Author: Stephan Zinkler
 */



#include "common.h"


int main(int argc, char* argv[]) {

	int ret;
    struct rte_mempool *mbuf_pool;

    /* Initialize the Environment Abstraction Layer (EAL). */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    /* Creates a new mempool in memory to hold the msend_bufs. */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
	MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	// Create Signal Action for interrupts
	struct sigaction newSigAction;
    memset((void *)&newSigAction, 0, sizeof(struct sigaction));
    newSigAction.sa_handler = &signal_handler;
    newSigAction.sa_flags = SA_NODEFER;

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

	sleep(1);

	setup_receiver(mbuf_pool);
	setup_sender(mbuf_pool);

    rte_eal_mp_wait_lcore();

    return 0;

}
