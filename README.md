# Introduction

This repository contains all artefacts developed as a part of Time-sensitive Software-defined Network (TSSDN), developed at the Distributed Systems Department, Insititue for Parallel and Distributed Systems (IPVS) at the university of Stuttgart.

TSSDN is based on software-defined networking and is used for imparting real-time guarantees (bounded end-to-end latency and jitter) for time-triggered communication flows in cyber-physical systems. Being an SDN based architecture, the control plane and data plane of TSSDN is separated with the control plane having a global view on the data plane, i.e., the underlying network topology and the communication flows in the network. The TSSDN control plane exploits this global view to compute transmission schedules for the end systems in the network such that the time-triggered traffic by these systems do not encounter any cross traffic. This eliminates the non-deterministic queueing delays, thus providing bounded end-to-end latency and jitter.

More information about the architecture and algorithms of TSSDN is available in our [technical report](http://goo.gl/8HtZXo).

# Folder Structure
- **DataPlane**: Contains the communication interfaces that were used in the data plane \(DPDK, Sockets etc.\).
- **Controller**: This folder contains the controller scripts for the control plane. We plan to develop TSSDN as a Ryu control module. However, this is yet to done. (Under development)
- **Scheduler**: The crux of TSSDN is its intelligent transmission scheduling algorithms. All the implemented scheduling algorithms are available in this folder. The scheduling algorithms are basically ILP formulations specified using PuLP (a python package for handling ILP solvers). We used CPLEX, an ILP solver from IBM, to solve these ILPs.

# Tasks
- [x] Online scheduling algorithms for time-triggered traffic (*Shortest Available Path* & *Mini-max*).
- [ ] Offline scheduling algorithms for time-triggered traffic (*Unconstrained Routing*, *Fixed-path Routing* & *Pathsets Routing*).
- [ ] Scheduling for event-triggered traffic.
- [ ] Integration with RYU, an SDN controller, so that all algorithms can be executed on the control plane.

# Required Packages
* Python Packages (Can be installed using pip)
  * Ryu
  * NetworkX
  * PuLP
  * Numpy (Used only for analysis)
* C Libraries
  * DPDK
* Miscelleneous
  * CPLEX (We used v12.5)

# Contact Details
For any further information, feel free to contact the authors of the above technical report.
