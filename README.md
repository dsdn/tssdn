# Introduction

This repository contains all artefacts developed as a part of Time-sensitive Software-defined Network (TSSDN), developed at the Distributed Systems Department, Insititue for Parallel and Distributed Systems (IPVS) at the university of Stuttgart.

TSSDN is based on software-defined networking and is used for imparting real-time guarantees (bounded end-to-end latency and jitter) for time-triggered communication flows in cyber-physical systems. Being an SDN based architecture, the control plane and data plane of TSSDN is separated with the control plane having a global view on the data plane, i.e., the underlying network topology and the communication flows in the network. The TSSDN control plane exploits this global view to compute transmission schedules for the end systems in the network such that the time-triggered traffic by these systems do not encounter any cross traffic. This eliminates the non-deterministic queueing delays, thus providing bounded end-to-end latency and jitter.

More information about the architecture and algorithms of TSSDN is available in this [technical report](ftp://ftp.informatik.uni-stuttgart.de/pub/library/ncstrl.ustuttgart_fi/TR-2016-03/TR-2016-03.pdf).


# Folder Structure




# Required Packages


