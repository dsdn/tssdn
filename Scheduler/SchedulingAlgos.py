#! /usr/bin/python
'''
Scheduling algorithms for TSSDN.
Implements two interface functions - 
    ShortestAvailablePath - Schedules over shortest available path
    MiniMax - Schedules using MiniMax algorithm

Author - Naresh Nayak
Date - 15.02.2016
'''

from pulp import *
import time
import numpy as np

current_time = lambda: int(round(time.time() * 1000))

class SchedulingAlgos():

    # Constructor
    def setTopoTrim(self, trim):
        self.trimTopo = trim

    # Scheduling function - ShortestAvailablePath
    def ShortestAvailablePath(self, topo, src, dst, ls):

        numDst = len(dst)

        # Number of slots on which scheduling is to be done
        timeSlots = ls.keys()

        # Optimization 1 - Trim the topology
        if self.trimTopo == 1:
            nodes = topo.nodes()
            for n in nodes:
                if topo.degree(n) <= 2 and (n != src) and (n not in dst):
                    topo.remove_node(n)
            links = topo.edges()
            for (n1, n2) in links:
                if n1 in dst or n2 == src: topo.remove_edge(n1,n2)      
  
        nodes = topo.nodes()
        links = topo.edges()

        # Create the ILP
        ilpCreation = -current_time()
        ilpProb = LpProblem("ShortestAvailablePath", LpMinimize)
         
        # Decision Variables
        # 1. Variables for link usage
        decVarLinks = LpVariable.dicts("LinkDec", links, cat = pulp.LpBinary)
        # 2. Variables for denote the number of intended destinations for the packet on the link.
        decVarLinkLoads = LpVariable.dicts("LinkLoadsDec", links, lowBound = 0, upBound = numDst, cat = pulp.LpInteger)
        # 3. Variables for slots 
        decVarSlots = LpVariable.dicts("SlotDec", timeSlots, cat = pulp.LpBinary)
        # 4. Variables representing the auxiliary variables
        decVar = []
        for l in links:
            for t in timeSlots:
                decVar.append((l,t))
        decVarLinkSlots = LpVariable.dicts("LinkSlotDec", decVar, cat = pulp.LpBinary)

        # Objective function
        # Minimize the length of the path on which the flow is routed
        ilpProb += sum(decVarLinks[l] for l in links)

        # Constraints
        # 1. Only one time-slot to be allocated
        ilpProb += sum(decVarSlots[s] for s in timeSlots) == 1, "Only one slot"

        # 2. For all nodes (except src and dst), sum of outgoing edges equal to incoming edges
        for n in nodes:
            outEdges = topo.out_edges(n)
            inEdges = topo.in_edges(n)

            if n == src:
                ilpProb += sum(decVarLinkLoads[e] for e in outEdges) == numDst, "Sum Out-Edges - "+n
                ilpProb += sum(decVarLinkLoads[e] for e in inEdges) == 0, "Sum In-Edges - "+n
            elif n in dst:  
                ilpProb += sum(decVarLinkLoads[e] for e in outEdges) == 0, "Sum Out-Edges - "+n
                ilpProb += sum(decVarLinkLoads[e] for e in inEdges) == 1, "Sum In-Edges - "+n
            else:
                ilpProb += sum(decVarLinkLoads[e] for e in outEdges) == sum(decVarLinkLoads[e] for e in inEdges), "Sum Edges - "+n

        # 3. Get which links are to be used based on the number of destinations for the packet
        for e in links:
            ilpProb += decVarLinks[e] >= decVarLinkLoads[e] / float(numDst)

        # 4. Relation between decVarLinks, decVarSlots and decVarLinkSlots
        for l in links:
            for t in timeSlots:
                ilpProb += decVarLinkSlots[(l,t)] <= decVarLinks[l],"Linear Constraint 1 - " + str((l,t))
                ilpProb += decVarLinkSlots[(l,t)] <= decVarSlots[t],"Linear Constraint 2 - " + str((l,t))
                ilpProb += decVarLinkSlots[(l,t)] >= decVarLinks[l] + decVarSlots[t] - 1, "Linear Constraint 3 - " + str((l,t))

        # 5. No collision on links
        for t in timeSlots:
            for l in links:
                ilpProb += ls[t][l] + decVarLinkSlots[(l,t)] <= 1, "Link collision - " + str(t) + " " + str(l)
        ilpCreation += current_time()

        # Solve the ILP        
        ilpSolution = -current_time()
        ilpProb.solve(CPLEX(msg=0))
        ilpSolution += current_time()
        slotAlloc = (0,0)

        # If the ILP is feasibly solved, we extract the allocated time-slot and the path
        if ilpProb.status == 1:
            route = []
            # For extraction of the values from the decision variables, we need an ugly hack.
            # Instead of checking if the assigned value by CPLEX is 1, we check if it is greater than 0.5.
            # This is because we observed that occassionally few variables were assigned real numbers (very low exponents) despite the 
            # ILP specifying otherwise. It might be a CPLEX bug or PuLP bug. So for now we use 0.5 for our checks.
            for t in timeSlots:
                if decVarSlots[t].varValue > 0.5:
                    slotAlloc = t
            
            for l in links:
                if decVarLinks[l].varValue > 0.5:
                    route.append(l)

            return (route, slotAlloc, ilpCreation, ilpSolution)
        else:
            return ([ilpProb.status], slotAlloc, ilpCreation, ilpSolution)

    # Scheduling function - ShortestAvailablePath
    def MiniMax(self, topo, src, dst, ls):

        numDst = len(dst)

        # Number of slots on which scheduling is to be done
        timeSlots = ls.keys()

        # Optimization 1 - Trim the topology
        if self.trimTopo == 1:
            nodes = topo.nodes()
            for n in nodes:
                if topo.degree(n) <= 2 and (n != src) and (n not in dst):
                    topo.remove_node(n)

            links = topo.edges()
            for (n1, n2) in links:
                if n1 in dst or n2 == src: topo.remove_edge(n1,n2)      
  
        nodes = topo.nodes()
        links = topo.edges()

        # Create the ILP
        ilpCreation = -current_time()
        ilpProb = LpProblem("Mini-Max", LpMinimize)

        # Decision Variables
        # 1. Variables for link usage
        decVarLinks = LpVariable.dicts("LinkDec", links, cat = pulp.LpBinary)
        # 2. Variables for denote the number of intended destinations for the packet on the link.
        decVarLinkLoads = LpVariable.dicts("LinkLoadsDec", links, lowBound = 0, upBound = numDst, cat = pulp.LpInteger)
        # 3. Variables for slots 
        decVarSlots = LpVariable.dicts("SlotDec", timeSlots, cat = pulp.LpBinary)
        # 4. Variables representing the auxiliary variables
        decVar = []
        for l in links:
            for t in timeSlots:
                decVar.append((l,t))
        decVarLinkSlots = LpVariable.dicts("LinkSlotDec", decVar, cat = pulp.LpBinary)
        # 5. Decision variable for maximum load
        decVarMaxLoad = LpVariable("MaxLoad", lowBound = 0, upBound = len(timeSlots), cat = pulp.LpInteger)

        # Objective function
        # Minimize the length of the path on which the flow is routed
        ilpProb += decVarMaxLoad + round(1/float(len(links)), 5) * (sum(decVarLinks[l] for l in links))

        # Constraints
        # 1. Only one time-slot to be allocated
        ilpProb += sum(decVarSlots[s] for s in timeSlots) == 1, "Only one slot"

        # 2. For all nodes (except src and dst), sum of outgoing edges equal to incoming edges
        for n in nodes:
            outEdges = topo.out_edges(n)
            inEdges = topo.in_edges(n)

            if n == src:
                ilpProb += sum(decVarLinkLoads[e] for e in outEdges) == numDst, "Sum Out-Edges - "+n
                ilpProb += sum(decVarLinkLoads[e] for e in inEdges) == 0, "Sum In-Edges - "+n
            elif n in dst:  
                ilpProb += sum(decVarLinkLoads[e] for e in outEdges) == 0, "Sum Out-Edges - "+n
                ilpProb += sum(decVarLinkLoads[e] for e in inEdges) == 1, "Sum In-Edges - "+n
            else:
                ilpProb += sum(decVarLinkLoads[e] for e in outEdges) == sum(decVarLinkLoads[e] for e in inEdges), "Sum Edges - "+n

        # 3. Get which links are to be used based on the number of destinations for the packet
        for e in links:
            ilpProb += decVarLinks[e] >= decVarLinkLoads[e] / float(numDst)

        # 4. Relation between decVarLinks, decVarSlots and decVarLinkSlots
        for l in links:
            for t in timeSlots:
                ilpProb += decVarLinkSlots[(l,t)] <= decVarLinks[l],"Linear Constraint 1 - " + str((l,t))
                ilpProb += decVarLinkSlots[(l,t)] <= decVarSlots[t],"Linear Constraint 2 - " + str((l,t))
                ilpProb += decVarLinkSlots[(l,t)] >= decVarLinks[l] + decVarSlots[t] - 1, "Linear Constraint 3 - " + str((l,t))

        # 5. No collision on links
        for t in timeSlots:
            for l in links:
                ilpProb += ls[t][l] + decVarLinkSlots[(l,t)] <= 1, "Link collision - " + str(t) + " " + str(l)

        # 6. Load on each link must be less than the maximum load
        for l in links:
            (l1, l2) = l
            if l1 != src and l2 not in dst: 
                ilpProb += sum([ls[t][l] for t in timeSlots]) + decVarLinks[l] <= decVarMaxLoad, "Link Loads - "+str(l)

        ilpCreation += current_time()

        # Solve the ILP        
        ilpSolution = -current_time()
        ilpProb.solve(CPLEX(msg=0))
        ilpSolution += current_time()
        slotAlloc = (0,0)

        # If the ILP is feasibly solved, we extract the allocated time-slot and the path
        if ilpProb.status == 1:
            route = []
            # For extraction of the values from the decision variables, we need an ugly hack.
            # Instead of checking if the assigned value by CPLEX is 1, we check if it is greater than 0.5.
            # This is because we observed that occassionally few variables were assigned real numbers (very low exponents) despite the 
            # ILP specifying otherwise. It might be a CPLEX bug or PuLP bug. So for now we use 0.5 for our checks.
            for t in timeSlots:
                if decVarSlots[t].varValue > 0.5:
                    slotAlloc = t
            
            for l in links:
                if decVarLinks[l].varValue > 0.5:
                    route.append(l)

            return (route, slotAlloc, ilpCreation, ilpSolution)
        else:
            return ([ilpProb.status], slotAlloc, ilpCreation, ilpSolution)   
