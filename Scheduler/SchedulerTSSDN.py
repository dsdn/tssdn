#! /usr/bin/python
'''
Implements the top level of TSSDN scheduler for incremental scheduling

Naresh Nayak
15.02.2016
'''

import time
import numpy as np
from threading import Condition
from collections import Counter
from SchedulingAlgos import SchedulingAlgos

current_time = lambda: int(round(time.time() * 1000))

# Top-level class for TSSDN Scheduling
# Wraps the scheduling algorithms
class SchedulerTSSDN:
    
    # Constants for the class
    SAP = 0                 # Shortest Available Path
    MM = 1                  # Mini-max 

    # Constructor
    def __init__(self, topo, sw, hosts):
        
        # Initialization
        self.topo = topo
        self.switches = sw
        self.hosts = hosts
        self.flowDB = []
        self.flowDBOptimal = []

        # Default configuration - SAP, 10 Slots, TT/TS Enabled with 1 attempt
        # Can be changed by invoking the interface functions
        self.scheduler = SchedulingAlgos()
        self.setAlgo(self.SAP)    
        self.setTotalSlots(10)
        self.setTopologyTrimming(1)
        self.setTimeslotSlicing(1,1)

        # Process the topology to derive edge links, core links and edge switches
        self.coreLinks = filter(lambda (x,y): (x in self.switches) and (y in self.switches), self.topo.edges())
        self.numCoreLinks = len(self.coreLinks)
        self.edgeLinks = list(set(self.topo.edges()) - set(self.coreLinks))
        self.hostToSwitch = {}
        for (x, y) in self.edgeLinks:
            if x in self.hosts: self.hostToSwitch[x] = y       

      
    # Prepare the scheduler and initialize the network states
    def schedulerConfigured(self):
        self.coreNetworkState = {}
        for t in self.timeSlots:
            self.coreNetworkState[t] = {}
            for e in self.coreLinks:
                self.coreNetworkState[t][e] = (0, [])

        self.edgeNetworkState = {}
        for t in self.timeSlots:
            self.edgeNetworkState[t] = {}
            for e in self.edgeLinks:
                self.edgeNetworkState[t][e] = (0, [])

    # Set algorithm for incremental scheduling
    def setAlgo(self, algo):
        self.algo = algo

    # Set total number of slots available for scheduling
    def setTotalSlots(self, slots):
        self.totalSlots = slots
        self.timeSlots = range(0, slots)

    # Set topology trimming
    #   1 - Topology trimming enabled
    #   0 - Topology trimming disabled
    def setTopologyTrimming(self, onOff):
        self.topologyTrimming = onOff
        self.scheduler.setTopoTrim(onOff)

    # Set timeslot slicing
    # Inputs - 
    # - Slize Size - If zero, then the timeslot slicing is turned off
    # - attempts - Total number of attempts at scheduling
    def setTimeslotSlicing(self, sliceSize, attempts):
        self.sliceSize = sliceSize
        self.maxAttempts = attempts
        if sliceSize > 0: self.timeslotSlicing = 1
        else: self.timeslotSlicing = 0

    # Function to get the link state for the given set of slots
    # Input - Set of slots, flow spec. (only period is used) and set of mandatory links (edge links for the host)
    # Output - 2 dim matrix - Link x (Slot, phase) -> {0, 1} 
    def getLinkState(self, selectedSlots, flow, mandatoryLinks):

        (s,d,p) = flow

        if self.topologyTrimming == 0: 
            mandatoryLinks = self.edgeLinks

        # Create link state for the selected slots
        ls = {}
        for (t,ph,n) in selectedSlots:
            state = {}
            # Insert state for all the core links
            for e in self.coreLinks:
                if self.coreNetworkState[t][e][0] == 0: state[e] = 0
                elif (self.coreNetworkState[t][e][0] == p) and (ph not in self.coreNetworkState[t][e][1]): state[e] = 0
                else: state[e] = 1
            # Insert state for the edge links 
            for e in mandatoryLinks:
                if self.edgeNetworkState[t][e][0] == 0: state[e] = 0
                elif (self.edgeNetworkState[t][e][0] == p) and (ph not in self.edgeNetworkState[t][e][1]): state[e] = 0
                else: state[e] = 1

            ls[(t, ph)] = state
        return ls  

    # Interface to update link state. Used to update existing schedules after flow is successfully scheduled.
    # CAUTION - For now only flow set-up is handled. Flow removal will be added in due course
    # Input - Flow, links over which the flow is routed, Slot allocated to the flow.
    def updateLinkState(self, flow, links, slot):
        
        (s,d,p) = flow
        (t, ph) = slot

        for l in links:
            if l in self.coreLinks: 
                usedPhases = [ph]
                usedPhases.extend(self.coreNetworkState[t][l][1])
                if self.coreNetworkState[t][l][0] != 0: 
                    assert(self.coreNetworkState[t][l][0] == p), "Period change in core link - " + str(t) + str(l)
                    assert(ph not in self.coreNetworkState[t][l][1]), "Phase already assigned to the core link - " + str(t) + str(l) 
                self.coreNetworkState[t][l] = (p, usedPhases)
            elif l in self.edgeLinks: 
                usedPhases = [ph]
                usedPhases.extend(self.edgeNetworkState[t][l][1])
                if self.edgeNetworkState[t][l][0] != 0: 
                    assert(self.edgeNetworkState[t][l][0] == p), "Period change in edge link - " + str(t) + str(l) 
                    assert(ph not in self.edgeNetworkState[t][l][1]), "Phase already assigned to the edge link - " + str(t) + str(l)
                self.edgeNetworkState[t][l] = (p, usedPhases)          

    # Interface to get slots for Scheduling
    # - flow - Information about the flow
    # - mandatoryLinks - List of mandatory links
    # - flagCoreFlow - If set, then core links will be examined
    def getSlots(self, flow, mandatoryLinks, flagCoreFlow):

        # Local function that analyzes suitability of time slot 't'
        def slotAnalysis(t, flow, mandatoryLinks, flagCoreFlow):

            numAvailableLinks = 0
            phase = 0
            availablePhases = []
            usedPhases = []

            (s, d, p) = flow

            # Check availability in mandatory links
            for e in mandatoryLinks:
                if self.edgeNetworkState[t][e][0] in [0, p]:
                    availablePhases.extend(range(0, p))
                    usedPhases.extend(self.edgeNetworkState[t][e][1])

            # Check how phase usage in the links.
            # Phases that are available in all links are available for consideration
            counterAvailablePhases = Counter(availablePhases)
            counterUsedPhases = Counter(usedPhases)
            counterAvailablePhases.subtract(counterUsedPhases)
            availablePhasesEdge = filter(lambda x: x[1] == len(mandatoryLinks), counterAvailablePhases.most_common())

            # If there are no available phases return available links as zero
            if not len(availablePhasesEdge):
                numAvailableLinks = 0
            else:
                numAvailableLinks += len(mandatoryLinks)
                # Default phase is the first available phase in the mandatory links
                phase = availablePhasesEdge[0][0]

                # If its a core flow then all core links must be checked to select a phase
                if flagCoreFlow:
        
                    # Sum of all the links that are completely unassigned during this timeslot
                    numAvailableLinks += self.coreNetworkState[t].values().count((0, []))

                    # Check for all the other links if they carry a flow with the same period
                    usedPhases = []
                    availablePhases = []
                    for e in self.coreLinks:
                        if self.coreNetworkState[t][e][0] == p: 
                            availablePhases.extend(range(0, p))
                            usedPhases.extend(self.coreNetworkState[t][e][1])

                    counterAvailablePhases = Counter(availablePhases)
                    counterUsedPhases = Counter(usedPhases)
                    counterAvailablePhases.subtract(counterUsedPhases)

                    for (ph, num) in counterAvailablePhases.most_common():
                        if ph in [h[0] for h in availablePhasesEdge]:
                            phase = ph
                            numAvailableLinks += num
                            break

            # Return the details of the slot
            return (t, phase, numAvailableLinks)

        # Flow description
        (s, d, p) = flow

        # Get slot occupancy of core links for all slots. 
        # If edge links are occupied at that slot it will be returned as occupied.
        slotOccupancy = map(lambda x: slotAnalysis(x, flow, mandatoryLinks, flagCoreFlow), self.timeSlots)
        selectedSlots = filter(lambda x: x[2] > 0, slotOccupancy)      
        selectedSlots = sorted(selectedSlots, key=lambda x: x[2],reverse=True)

        return selectedSlots
        
    # Interface called when flow arrives
    def scheduleFlow(self, flow):

        # Timers to measure execution time
        schedTime = -current_time()

        # Initializations
        (s, d, p) = flow

        links = []
        slot = (0, 0)
        ilpSpec, ilpSol = -1, -1
        attempt = 0

        # Detect if the flow to be scheduled is an edge flow or a core flow
        # Edge flow uses only edge links, while core flow uses core links as well in addition to an edge link
        edgeDstSwitches = [self.hostToSwitch[h] for h in d]
        edgeSrcSwitches = [self.hostToSwitch[h] for h in [s]]
        if len(set(edgeDstSwitches + edgeSrcSwitches)) == 1: 
            flagCoreFlow = False
        else:
            flagCoreFlow = True

        # Get the set of mandatory links
        mandatoryLinks = [(self.hostToSwitch[h], h) for h in d]
        mandatoryLinks.append((s, self.hostToSwitch[s]))
        
        # For an edge flow the slot can be assigned directly
        if not flagCoreFlow:

            # Get the set of slots on which scheduling is to be done
            bestSlots = self.getSlots(flow, mandatoryLinks, flagCoreFlow)   

            # Slots are available
            if len(bestSlots):
                attempt += 1                
        
                # Allocate the slot
                slot = bestSlots[0][:-1]
                ilpSpec, ilpSol = -1, -1
                links = mandatoryLinks

                # Update the network state
                self.updateLinkState(flow, mandatoryLinks, slot)

        # For core flows
        else:

            # Execute with timeslot slicing
            if self.timeslotSlicing:
                index = 0
                width = self.sliceSize

                # Get the set of slots on which scheduling is to be done
                bestSlots = self.getSlots(flow, mandatoryLinks, flagCoreFlow)   

                # Slots are available
                if len(bestSlots):                    

                    # Keep trying to schedule till a schedule is found or slots are exhausted
                    while index < len(bestSlots) and attempt < self.maxAttempts:
                        attempt += 1                          
                        scheduleSlots = bestSlots[index:index + width]

                        # Create the link state data structure for the obtained slots
                        ls = self.getLinkState(scheduleSlots, flow, mandatoryLinks)

                        # Execute the ILP's as configured
                        if (self.algo == SchedulerTSSDN.SAP):
                            (links, slot, ilpSpec, ilpSol) = self.scheduler.ShortestAvailablePath(self.topo.copy(), s, d, ls)
                        elif (self.algo == SchedulerTSSDN.MM):
                            (links, slot, ilpSpec, ilpSol) = self.scheduler.MiniMax(self.topo.copy(), s, d, ls)

                        # Update the link state if schedule is found
                        if len(links) > 1: 
                            self.updateLinkState(flow, links, slot)  
                            break

                        # Schedule not found, so try next indexes
                        index = index + width

            else: # No timeslot slicing

                attempt += 1

                # Create the set of slots
                slotsSet = []
                for t in self.timeSlots:
                    for ph in range(0, p):
                        slotsSet.append((t,ph, 0))

                # Create the link state data structure for the obtained slots
                ls = self.getLinkState(slotsSet, flow, mandatoryLinks)

                # Execute the ILP's as configured
                if (self.algo == SchedulerTSSDN.SAP):
                    (links, slot, ilpSpec, ilpSol) = self.scheduler.ShortestAvailablePath(self.topo.copy(), s, d, ls)
                elif (self.algo == SchedulerTSSDN.MM):
                    (links, slot, ilpSpec, ilpSol) = self.scheduler.MiniMax(self.topo.copy(), s, d, ls)

                # Update the link state if schedule is found
                if len(links): 
                    self.updateLinkState(flow, links, slot)

        schedTime += current_time()

        # Update the flow database
        self.flowDB.append((flow, links, slot, ilpSpec, ilpSol, schedTime, flagCoreFlow, attempt))

    # Interface called when flow arrives to check the optimal solution (DEBUG function)
    # This function only computes the optimal. It does not update the link states.
    # !!! CAUTION !!! - Use this function only for comparison of generated schedule with optimal.
    def scheduleFlowOptimally(self, flow):

        # Timers to measure execution time
        schedTime = -current_time()

        # Initializations
        (s, d, p) = flow

        links = []
        slot = (0, 0)
        ilpSpec, ilpSol = -1, -1
        attempt = 0

        # Detect if the flow to be scheduled is an edge flow or a core flow
        # Edge flow uses only edge links, while core flow uses core links as well in addition to an edge link
        edgeDstSwitches = [self.hostToSwitch[h] for h in d]
        edgeSrcSwitches = [self.hostToSwitch[h] for h in [s]]
        if len(set(edgeDstSwitches + edgeSrcSwitches)) == 1: 
            flagCoreFlow = False
        else:
            flagCoreFlow = True

        # Get the set of mandatory links
        mandatoryLinks = [(self.hostToSwitch[h], h) for h in d]
        mandatoryLinks.append((s, self.hostToSwitch[s]))
        
        # For an edge flow the slot can be assigned directly
        if not flagCoreFlow:

            # Get the set of slots on which scheduling is to be done
            bestSlots = self.getSlots(flow, mandatoryLinks, flagCoreFlow)   

            # Slots are available
            if len(bestSlots):
                attempt += 1                
        
                # Allocate the slot
                slot = bestSlots[0][:-1]
                ilpSpec, ilpSol = -1, -1
                links = mandatoryLinks

        # For core flows
        else:
            attempt += 1

            # Create the set of slots
            slotsSet = []
            for t in self.timeSlots:
                for ph in range(0, p):
                    slotsSet.append((t,ph, 0))

            # Create the link state data structure for the obtained slots
            ls = self.getLinkState(slotsSet, flow, mandatoryLinks)

            # Execute the ILP's as configured
            if (self.algo == SchedulerTSSDN.SAP):
                (links, slot, ilpSpec, ilpSol) = self.scheduler.ShortestAvailablePath(self.topo.copy(), s, d, ls)
            elif (self.algo == SchedulerTSSDN.MM):
                (links, slot, ilpSpec, ilpSol) = self.scheduler.MiniMax(self.topo.copy(), s, d, ls)

        schedTime += current_time()

        # Update the flow database
        self.flowDBOptimal.append((flow, links, slot, ilpSpec, ilpSol, schedTime, flagCoreFlow, attempt))

    # Function to get the flow database
    # Can be used to check the number of flows scheduled and the corresponding execution times
    def getFlowDatabase(self):
        return self.flowDB

    # Function to get the optimal flow database
    # Can be used to compare with actual flow database to determine sub-optimalities and false negatives
    def getOptimalFlowDatabase(self):
        return self.flowDBOptimal

