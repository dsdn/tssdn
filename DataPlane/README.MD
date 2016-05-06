# Introduction

This folder contains the C implementation of the communication interfaces (DPDK and Sockets) used for TSSDN. Each of these implementation has a sender application and a receiver application. The sender sends time-triggered (periodic) messages with the current time-stamp to the receiver. The receiver computes end-to-end latency for the messages and later puts it on a log file. Our implementation basically tests the suitability of the communication primitives for time-triggered networks, like TSSDN. In our future work, we will extend the usage of these interfaces for live cyber-physical systems. 

# Folder Structure

- Socket: Contains C programs (sender & receiver) that use socket interfaces
- DPDK: Contains C programs (sender, receiver & sender_receiver) that use DPDK interfaces.

# Prerequisites

- Implementations tested on Linux machines.
- Intel's DPDK must be installed on the machine. Refer to the [installation details](http://dpdk.org/download).

 
# Building the applications

- Socket Applications: Execute the makefiles in the folder
- DPDK Applications: This implementation offers both a sender/receiver only as well as a sender_receiver. The sender_receiver is required if the sender and receiver network interfaces are on the same machine as only one DPDK application can be executed on a machine at a given time.


# Running the applications

- Socket Applications: 

	sudo nice -adjustment=-20 ./receiver_c <port_num>

	sudo nice -adjustment=-20 ./sender_c <dest_ip_addr> <dest_port_num>

- DPDK Applications:
After make, dpdk has to be run with the application as an input parameter. Running the sender_receiver app would for instance look like this when ran from the DPDK folder:

	sudo nice -adjustment=-20 sender_receiver/build/app/sender_receiver -0xe -n4
	
where sender_receiver is the dpdk app to be ran and 0xe is the bitmask for which cores are to be used.

Parameters of the DPDK application can be configured by changing the definitions in common/config.h

- **DEBUG:**			0 means no debug messages are printed 

- **SEND_PERIOD_SEC:**	The number of seconds for the sending intervall	
- **SEND_PERIOD_USEC:** The number of microseconds for the sending intervall	
- **SEND_PERIOD_NSEC:** The number of nanoseconds for the sending interval calculated from USEC

- **NUM_SAMPLES:** 		Number of Packets to be sent

- **VLAN_PKT:**   		If 1, VLAN field is added to the packet
- **VLAN_TAG:**   		The VLAN tag ID
- **VLAN_PRIO:**  		Priority of the VLAN tag
- **DSCP:**       		The DSCP priority of the packet

- **TX_PORT_NO:** 		Port for transmitting packets (output)
- **RX_PORT_NO:**		Port for receiving packets (input)
- **TX_CORE_NR:**		The CPU core the sender task should run on
- **RX_CORE_NR:**		The CPU core the receiver task should run on

# Results
We measured the end-to-end latencies for 10,000 samples between the a sender and a receiver using DPDK and sockets. The latency variance is shown below: 
![Latencies](https://ipvs.informatik.uni-stuttgart.de/cloud/index.php/s/ET0PLSlmnkM94Wz)
