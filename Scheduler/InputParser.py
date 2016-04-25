#! /usr/bin/python
'''
Parses the input files and creates the data structures for the ILP 
Input - 
    links.dat -> (n1, n2)
    flows.dat -> (src, dst, period)

Author - Naresh Nayak
Date - 22.07.2015
'''

import networkx as nx

class Parser(object):
    
    # Constructor
    def __init__(self, loc):
        self.loc = loc
        self.flows = []
        self.topo = nx.DiGraph()
        self.switches = []
        self.hosts = []
        
        # Open the files
        fLinks = open(self.loc + "/links.dat", "r")
        fFlows = open(self.loc + "/flows.dat", "r")

        # Read the files
        linesLinks = fLinks.readlines()
        for l in linesLinks:
            tokens = l[1:-2].split(" ")
            self.topo.add_edge(tokens[0], tokens[1])
        fLinks.close()
        
        self.switches = self.topo.nodes()
        for n in self.topo.nodes():
            if (n[0] == 'h'): self.switches.remove(n)
        self.hosts = list(set(self.topo.nodes()) - set(self.switches))

        linesFlows = fFlows.readlines()
        for l in linesFlows:
            tokens = l[1:-2].split(" ")
            s = tokens[0]
            d = tuple(tokens[1:-1])
            p = int(tokens[-1])
            self.flows.append((s,d,p))
        fFlows.close()
