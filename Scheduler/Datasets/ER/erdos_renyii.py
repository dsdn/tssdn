#! /usr/bin/python
'''
Generates a random regular graph and creates a random set of flows with period
Output - 
    links.dat -> (n1, n2)
    flows.dat -> (src, dst, p)
    readme

Author - Naresh Nayak
Date - 15.02.2016
'''

import networkx as nx
import random, os, sys

# Input data required for topology generation
nSwitches = 15
nHosts = 50
nP = 0.95
nFlows = 100
maxPeriod = 1
maxDst = 1
nSeed = 0   
coreFlowsOnly = 1

# A. Build a random graphs
randGraph = nx.erdos_renyi_graph(nSwitches, nP, nSeed).to_directed()
random.seed(nSeed)

# B. Construct required input data for ILP
# B.1 -> Random topology corresponding to the random graph
hosts = []
switches = []
flows = []
flowHosts = []
hostToSwitch = {}

topo = nx.DiGraph()
for h in range(0, nHosts):
    hosts.append("h"+str(h))

for s in range(0, nSwitches):
    switches.append("s"+str(s))

for (u,v) in randGraph.edges():
    topo.add_edge("s"+str(u), "s"+str(v))

for h in range(0, len(hosts)):
    sw = random.randint(0, len(switches)-1)
    hostToSwitch["h"+str(h)] = sw
    topo.add_edge("h"+str(h), "s"+str(sw))
    topo.add_edge("s"+str(sw), "h"+str(h))
links = topo.edges()

while len(flows) < nFlows:

    r = random.randint(0,len(hosts)-1)
    hostSrc = "h"+str(r)

    numDst = random.randint(1, maxDst)
    hostDst = []
    while len(hostDst) < numDst:
        r = random.randint(0,len(hosts)-1)
        dst = "h"+str(r)
        if (hostSrc != dst) and dst not in hostDst: 
            hostDst.append(dst)

    edgeDstSwitches = [hostToSwitch[h] for h in hostDst]
    edgeSrcSwitches = [hostToSwitch[h] for h in [hostSrc]]
    if len(set(edgeDstSwitches + edgeSrcSwitches)) == 1 and coreFlowsOnly == 1: 
        proceed = False
    else:
        proceed = True

    if proceed:
        if (hostSrc, set(hostDst)) not in flowHosts:
            flow = (hostSrc, hostDst, random.randint(1, maxPeriod))
            flows.append(flow)
            flowHosts.append((hostSrc, set(hostDst)))

# Create directory name
strDir = "ER-s"+str(nSwitches)+"-h"+str(nHosts)+"-p"+str(nP)+"-f"+str(nFlows)+"-mp"+str(maxPeriod)+"-md"+str(maxDst)+"-seed"+str(nSeed)+"-c"+str(coreFlowsOnly)
try:
    os.mkdir(strDir)
except OSError:
    pass    

# Write links.dat
fLinks = open(strDir+"/links.dat", "w")
for (u,v) in links:
    fLinks.write("(" + u + " " + v + ")\n")
fLinks.close()

# Write flows.dat
fFlows = open(strDir+"/flows.dat", "w")
for (src, dst, p) in flows:
    strDst = ""
    for h in dst:
        strDst += h + " "
    fFlows.write("(" + src + " " + strDst[:-1] + " " + str(p) +")\n")
fFlows.close()
