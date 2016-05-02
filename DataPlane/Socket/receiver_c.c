/*
Opens a UDP socket on the port specified in the parameters of the commandline.
Receives the packets and prints the data rate at regular intervals

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


/* Globals */
#define NUM_SAMPLES 10000
#define PAYLOAD_SIZE 8
volatile int recvPkt = 1;

/* Local Functions */
static void signal_handler(int sig) {

    if (sig == SIGINT) {

        recvPkt = 0;
    }
}

/* Main Function */
int main(int argc, char *argv[]) {

    // Local variables
    int socketNo, portNo, sampleIndex = 0, i;
    char *ipAddr;
    struct sockaddr_in self_addr;
    char buf[PAYLOAD_SIZE];
    char fileLog[32];
    unsigned long int sentTime, recdTime;
    unsigned long int sentTimes[NUM_SAMPLES];
    unsigned long int latency[NUM_SAMPLES];
    struct timeval timeval;
    FILE *fLog;

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

    if (bind(socketNo, (struct sockaddr *)&self_addr, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "Error binding socket - %s\n", strerror(errno));
        exit(0);
    }

    // Attach signal handlers
    if (sigaction(SIGINT, &newSigAction, NULL) < 0) {
        fprintf(stderr, "Error attaching signal handler.\n");
        exit(0);
    }

    // Open a file to log timings
    gettimeofday(&timeval, NULL);
    recdTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
    sprintf(fileLog, "log-%ld", recdTime);
    fLog = fopen(fileLog, "w+");

    while(recvPkt && (sampleIndex < NUM_SAMPLES)){
        //try to receive some data, this is a blocking call
        if (read(socketNo, buf, PAYLOAD_SIZE) > 0) {

            if (gettimeofday(&timeval, NULL) >= 0) {
                recdTime = timeval.tv_sec * pow(10, 6) + timeval.tv_usec;
                memcpy(&sentTime, buf, sizeof(unsigned long int));
                sentTimes[sampleIndex] = sentTime;
                latency[sampleIndex] = recdTime - sentTime;
                sampleIndex++;
            }            
        }
    }
    
    for (i = 0; i < sampleIndex; i++)
        fprintf(fLog, "%ld %ld\n", sentTimes[i], latency[i]);
    fclose(fLog);
}

