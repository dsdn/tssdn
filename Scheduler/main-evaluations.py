#! /usr/bin/python
'''
Main file for the evaluations.
Takes as input the dataset and creates an instance of scheduler and schedules flow one by one.

Author - Naresh Nayak
Date - 24.04.2016
'''

import sys,socket
from InputParser import Parser
from SchedulerTSSDN import SchedulerTSSDN
import  numpy as np

# Command line inputs
# 1 - Location of the dataset
# 2 - Type of algorithm 0 - SAP, 1 - MM
# 3 - Total number of slots available
# 4 - Topology trimming
# 5 - Slice size
# 6 - Attempts
# 7 - Compute optimal or not

# Get the location of the dataset and parse it
# 1 - Location of the dataset
loc = sys.argv[1]
dataset = Parser(loc)
tssdn = SchedulerTSSDN(dataset.topo, dataset.switches, dataset.hosts)

# Set the used algorithm
tssdn.setAlgo(int(sys.argv[2]))

# Set total number of used slots
tssdn.setTotalSlots(int(sys.argv[3]))

# Set topology trimming
tssdn.setTopologyTrimming(int(sys.argv[4]))

# Set topology slicing
if len(sys.argv) > 5:
    sliceSize = int(sys.argv[5])
    tssdn.setTimeslotSlicing(sliceSize, int(sys.argv[6]))
else:
    sliceSize = 0
tssdn.schedulerConfigured()

# Optimality calculations
optimalCalc = int(sys.argv[7])

# Schedule all flows in the dataset
for f in dataset.flows:
    print f, dataset.flows.index(f) + 1
    if optimalCalc > 0: tssdn.scheduleFlowOptimally(f)
    tssdn.scheduleFlow(f)

# Get the flow database and print it in a file
flowDB = tssdn.getFlowDatabase()
flowDBOpt = tssdn.getOptimalFlowDatabase()

# The filenames for the log are derived from the dataset and the configurations
fName = loc.split("/")
fNameStr = "log-"+fName[-2]+"-slots"+str(tssdn.totalSlots)+"-algo"+str(tssdn.algo)+"-"+str(tssdn.topologyTrimming)+"-"+str(tssdn.sliceSize)+"-"+str(tssdn.maxAttempts)+"-"+socket.gethostname()
fLog = open(fNameStr, "w")

# Analyze the database for the number of scheduled (core/edge) flow, number of failed ILP attempts, sub-optimals and false negatives.
# We analyze the number of sub-optimals and false-negatives only when time-slicing is enabled.
numSchedFlows = 0
numFlows = len(flowDB)
numEdgeFlows = 0
numCoreFlows = 0
numSchedEdgeFlows = 0
numSchedCoreFlows = 0    
numIlpFailures = 0
numSubOptimal = 0
numFalseNegative = 0

accSchedTimes = []
accSubOptimal = []
accFalseNegative = []

for i in range(0, len(flowDB)):
    flow = flowDB[i][0]
    links = flowDB[i][1]
    slot = flowDB[i][2]
    ilpSpec = flowDB[i][3]
    ilpSol = flowDB[i][4]
    totalTime = flowDB[i][5]
    coreFlow = flowDB[i][6]
    attempt = flowDB[i][7]

    if optimalCalc > 0:
        flowOpt = flowDBOpt[i][0]
        linksOpt = flowDBOpt[i][1]
        slotOpt = flowDBOpt[i][2]
        ilpSpecOpt = flowDBOpt[i][3]
        ilpSolOpt = flowDBOpt[i][4]
        totalTimeOpt = flowDBOpt[i][5]
        coreFlowOpt = flowDBOpt[i][6]
        attemptOpt = flowDBOpt[i][7]

    print >> fLog, (flow, links, "Length-"+str(len(links)), slot, ilpSpec, ilpSol, totalTime, coreFlow, "Attempt-"+str(attempt))
    if optimalCalc > 0:
        print >> fLog, (flowOpt, linksOpt, "Length-"+str(len(linksOpt)), slotOpt, ilpSpecOpt, ilpSolOpt, totalTimeOpt, coreFlowOpt, "Attempt-"+str(attemptOpt))

    accSchedTimes.append(totalTime)

    if not coreFlow:  
        numEdgeFlows += 1
    else: numCoreFlows += 1

    if len(links) > 1: 
        numSchedFlows += 1             
        if coreFlow: numSchedCoreFlows += 1
        else: numSchedEdgeFlows += 1
    elif (ilpSpec, ilpSol) != (-1,-1):
        numIlpFailures += 1

    if optimalCalc > 0:
        if len(links) > len(linksOpt): 
            print>>fLog, "Suboptimal\n"
            numSubOptimal += 1
        elif len(links) == len(linksOpt): 
            print>>fLog, "Optimal\n"
        elif len(links) < 2: 
            print>>fLog, "False-negative\n"
            numFalseNegative += 1
        else:
            print>>fLog, "Better than Optimal\n"

        accSubOptimal.append(numSubOptimal)
        accFalseNegative.append(numFalseNegative)

print>>fLog, numFlows, numEdgeFlows, numCoreFlows, numSchedEdgeFlows, numSchedCoreFlows, numIlpFailures, np.mean(accSchedTimes), np.std(accSchedTimes), max(accSchedTimes)
if optimalCalc > 0:
    print>>fLog,"Suboptimally Routed - " + str(numSubOptimal)
    print>>fLog,[i for i in accSubOptimal]
    print>>fLog,[i for i in accFalseNegative]



    
    
