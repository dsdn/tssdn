# Introduction

This branch contains the C implementation of the dataplane using dpdk.

# Prerequesites

- dpdk must be installed on the machine
- The ports to be used have to be connected to dpdk
 
# Making the Application

This implementation offers both a sender/receiver only as well as a sender plus receiver running on the same machine build.

To make the desired application, just call make [all/sender/receiver/sender_receiver] from the DPDK folder or cd into the respective folder and call make there.

# Running the Application

After make, dpdk has to be run with the application as an input parameter. Running the sender_receiver app would for instance look like this when ran from the DPDK folder:

	sudo nice -adjustment=-20 sender_receiver/build/app/sender_receiver -0xe -n4
	
where the adjustment defines the priority of the task (here: max), sender_receiver is the dpdk app to be ran and 0xe is the bitmask for which cores are to be used.

# Configuring the Application

Parameters of the application can be configured by changing the definition in common/config.h

- **DEBUG:**			0 means no debug messages are printed 

- **SEND_PERIOD_SEC:**	The number of seconds for the sending intervall	
- **SEND_PERIOD_USEC:** The number of microseconds for the sending intervall	
- **SEND_PERIOD_NSEC:** The number of nanoseconds for the sending intervall	calculated from USEC

- **NUM_SAMPLES:** 		Number of Packets to be sent. -1 means infinite

- **VLAN_PKT:**   		If 1, VLAN field is added to the packet
- **VLAN_TAG:**   		The VLAN tag ID
- **VLAN_PRIO:**  		Priority of the VLAN tag
- **DSCP:**       		The DSCP priority of the packet


- **TX_PORT_NO:** 		Port for transmitting packets (output)
- **RX_PORT_NO:**		Port for receiving packets (input)
- **TX_CORE_NR:**		The CPU core the sender task should run on
- **RX_CORE_NR:**		The CPU core the receiver task should run on
