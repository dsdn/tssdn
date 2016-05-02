/*
Opens a UDP socket on the port specified in the parameters of the commandline.
Receives the packets and prints the data rate at regular intervals

Author - Naresh Nayak
Date - 5.07.2015
*/

/* Include files */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

#define THROUGHPUT_INTERVAL 10


/* Globals */
volatile int recvPkt = 1;
uint64_t pktCount = 0;
uint64_t lastRead = 0;
int recvPkts = 1;

/* Local Functions */
static void signal_handler(int sig)
{
    uint64_t temp;
    uint64_t pktRecd;
    if (sig == SIGINT)
    {
        recvPkts = 0;
    }
    else if (sig == SIGALRM)
    {
        temp = pktCount;
        if (temp < lastRead) {
            pktRecd = pow(2, 64) + temp - lastRead;
        } else {
            pktRecd = temp - lastRead;
        }
        float rate = (float) (pktRecd / THROUGHPUT_INTERVAL);
        printf("Receiving rate in Mpps - %.2f\n", rate / pow(10,6));
        lastRead = temp;     
        alarm(THROUGHPUT_INTERVAL); 
    }
}

/* Main Function */
int main(int argc, char *argv[])
{
    // Local variables
    int socketNo, portNo;
    char *ipAddr;
    struct sockaddr_in self_addr;
    char buf[64];
    uint64_t payload;

    struct sigaction newSigAction;
    memset((void *)&newSigAction, 0, sizeof(struct sigaction));
    newSigAction.sa_handler = &signal_handler;
    newSigAction.sa_flags = SA_NODEFER;

    /* Parse the arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage %s portNo\n", argv[0]);
        exit(0);
    } else {
        portNo = atoi(argv[1]);
        ipAddr = argv[2];
    }

    /* Open and bind a socket to listen on the aformentioned port No. */
    socketNo = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketNo < 1) {
        fprintf(stderr, "Error in opening the socket.\n");
        exit(0);
    }

    memset((char*) &self_addr, 0, sizeof(sockaddr_in));
    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    self_addr.sin_port = htons(portNo);

    if (bind(socketNo, (struct sockaddr *)&self_addr, sizeof(struct sockaddr_in)) < 0){
        fprintf(stderr, "Error binding socket - %s\n", strerror(errno));
        exit(0);
    }

    // Attach signal handlers
    if (sigaction(SIGINT, &newSigAction, NULL) < 0) {
        fprintf(stderr, "Error attaching signal handler.\n");
        exit(0);
    }
    if (sigaction(SIGALRM, &newSigAction, NULL) < 0) {
        fprintf(stderr, "Error attaching signal handler.\n");
        exit(0);
    }

    alarm(THROUGHPUT_INTERVAL);

    while(recvPkts){
        //try to receive some data, this is a blocking call
        if (read(socketNo, buf, 64) > 0)
        {
            memcpy(&payload, buf, sizeof(uint64_t)); 
            pktCount++;      
        }
    }
}


