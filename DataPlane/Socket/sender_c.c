/*
Opens a UDP socket to send packets to the destination as mentioned in commandline arguments.
Sends the packets and prints the data rate at regular intervals.

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

#define THROUGHPUT_INTERVAL 10

/* Globals */
volatile int sendPkt = 1;
uint64_t pl = 0;
uint64_t lastRead = 0;


/* Local Functions */
static void signal_handler(int sig)
{
    uint64_t temp;
    uint64_t pktSent;
    if (sig == SIGINT)
    {
        sendPkt = 0;
    }
    else if (sig == SIGALRM)
    {
        temp = pl;
        if (temp < lastRead) {
            pktSent = pow(2, 64) + temp - lastRead;
        } else {
            pktSent = temp - lastRead;
        }
        float rate = (float) (pktSent / THROUGHPUT_INTERVAL);
        printf("Sending rate in Mpps - %.2f\n", rate / pow(10,6));
        lastRead = temp;     
        alarm(THROUGHPUT_INTERVAL); 
    }
}

/* Main Function */
int main(int argc, char *argv[])
{
    // Local variables
    int socketNo, portNo;
    char* ipAddr;
    struct hostent *dst;
    struct sockaddr_in dst_addr;
    char buf[8];

    struct sigaction newSigAction;
    memset((void *)&newSigAction, 0, sizeof(struct sigaction));
    newSigAction.sa_handler = &signal_handler;
    newSigAction.sa_flags = SA_NODEFER;

    // Parse commandline arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s DstIP DstPort\n", argv[0]);
        exit(0);
    } else {
        portNo = atoi(argv[2]);
        ipAddr = argv[1];
    }

    // Open a socket
    socketNo = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketNo < 0) {
        fprintf(stderr, "Error opening socket");
        exit(0);
    }  
    int tos = 56; 
    setsockopt(socketNo, SOL_SOCKET, SO_PRIORITY, &tos, sizeof(tos));

    // Get the destionation  
    dst = gethostbyname(ipAddr);
    if (dst == NULL) {
        fprintf(stderr, "Error getting the host.\n");
        exit(0);
    }

    // Bind the socket for the destionation
    memset((char *) &dst_addr, 0, sizeof(dst_addr));
    dst_addr.sin_family = AF_INET;
    memcpy((char *)&dst_addr.sin_addr.s_addr, (char *)dst->h_addr, dst->h_length);
    dst_addr.sin_port = htons(portNo);

    if (connect(socketNo, (struct sockaddr*)&dst_addr, sizeof(dst_addr)) < 0) {
        fprintf(stderr, "Error binding socket");
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
    while(sendPkt)
    {
        memcpy(buf, (void*)&pl, sizeof(pl));
        pl++;
        write(socketNo, buf, sizeof(pl));
    }    

    // Return 0
    return 0;
}


