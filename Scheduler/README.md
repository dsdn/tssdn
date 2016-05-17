# Introduction
This folder contains the *incremental* scheduling algorithms of TSSDN and all the datasets used for evaluating the performance of the scheduling algorithms. The offline scheduling algorithms mentioned in [TR](http://goo.gl/8HtZXo) will be integrated in due course. Currently, two scheduling algorithms are implemented for TSSDN, *Shortest Available Path* & *Mini-max*. Both these algorithms are implemented in the [file](SchedulingAlgos.py).

The datasets (different topologies and sets of flows) which we used for our evaluations are present in the [Datasets](Datasets) folder. We mainly used three topology models to create randomized graphs for our evaluations, i.e., Erdos-Renyii, Barabasi-Albert and Random Regular graphs. The repository also contains the scripts used to generate the datasets. Note that the values of the parameters need to be manually changed for generating new datasets. Each evaluation scenario contains a folder containing two files, links.dat and flows.dat. The file on [links](./Datasets/RRG/RRG-s10-h50-d3-f100-mp1-md1-seed0-c1/links.dat) describe the links in the topology as directed edges, while the file on [flows](./Datasets/RRG/RRG-s10-h50-d3-f100-mp1-md1-seed0-c1/flows.dat) specify the flows. Flows are specified as a tuple describing the source host, set of destination hosts and transmission period of the flow. The name of the parent folder of the evaluation scenario encodes the set-up (topology and the type of flows being scheduled), for e.g., RRG-s10-h50-d3-f100-mp1-md1-seed0-c1 implies it is a random regular graph (degree 3) with 10 switches and 50 hosts with 100 flows (unicast with transmission periods as base-periods).

We also provide a python file (./main-evaluations.py) that executes the developed scheduling algorithms on the datasets. On execution of this python file, the flows are taken up for scheduling one after the other. At the end a log file is created (name of the scenario is added as postfix) containing the path over which each flow is routed and the time taken by the scheduler to compute the schedule. This file needs 7 command line inputs. 
- Location of the evaluation scenario to be executed.
- Type of algorithm - *Shortest Available Path* (0) or *Mini-max* (1)
- Number of time-slots over which scheduling is to be done
- Enable(1)/Disable(0) Topology Trimming
- Size of time-slot slice (0 if slicing is not enabled)
- Number of scheduling attempts
- Compute optimal for comparison (1 - Enable, 0 - Disable)
 
The following command executed evaluation scenario RRG-s10-h50-d3-f100-mp1-md1-seed0-c1 with 50 time-slots for scheduling using *Shortest Available Path* with time-slot slice of size 3.
```
./main-evaluations.py ./Datasets/RRG/RRG-s10-h50-d3-f100-mp1-md1-seed0-c1/flows.dat 0 50 1 3 1 0
```

A [sample log file](./log-RRG-s10-h50-d3-f100-mp1-md1-seed0-c1-slots50-algo0-1-3-1) available after execution of the scenario is available in the repository.
