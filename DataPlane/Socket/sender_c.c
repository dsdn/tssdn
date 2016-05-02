/*
Opens a UDP socket to send packets to the destination as mentioned in commandline arguments.
Sends the packets and prints the data rate at regular intervals.

Author - Naresh Nayak
Date - 5.07.2015
*/


/* Include files */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

#define SEND_PERIOD_USEC 10000
#define SEND_PERIOD_SEC 0
#define PAYLOAD_SIZE 1458

/* Globals */
volatile int sendPkt = 1;
int socketNo;
static char buf[PAYLOAD_SIZE];

/* Local Functions */
static void signal_handler(int sig)
{
    unsigned long int time = 0;
    struct timeval timeval;

    if (sig == SIGINT) {
        sendPkt = 0;

    } else if (sig == SIGALRM) {
    
        // Send the current time 
        if (gettimeofday(&timeval, NULL) >= 0) {

            time = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
            memcpy(buf, (void*)&time, sizeof(unsigned long int));
            write(socketNo, buf, PAYLOAD_SIZE);
        } else {
            
            fprintf(stderr, "Error acquiring timestamp - %s\n", strerror(errno));
            exit(0);
        }
    }
}

/* Main Function */
int main(int argc, char *argv[])
{
    // Local variables
    int portNo;
    char* ipAddr;
    struct hostent *dst;
    struct sockaddr_in dst_addr;
    struct itimerval interval;

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

    interval.it_interval.tv_sec = SEND_PERIOD_SEC;
    interval.it_interval.tv_usec = SEND_PERIOD_USEC;
    interval.it_value.tv_sec = SEND_PERIOD_SEC;
    interval.it_value.tv_usec = SEND_PERIOD_USEC;
    setitimer(ITIMER_REAL, &interval, NULL);

    while(sendPkt) {
            // Do Nothing
    }    

    // Return 0
    return 0;
}
