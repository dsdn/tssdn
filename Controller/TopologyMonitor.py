'''
Ryu Script for monitoring the underlying topology and creating a NetworkX DiGraph object.
This script throws the event EventTopologyChanged when link/switch is lost/detected.

Naresh Nayak
10.02.2016
'''

from ryu.base import app_manager
from ryu.controller.event import EventBase
from ryu.controller import ofp_event
from ryu.ofproto import ofproto_v1_4
from ryu.controller.handler import set_ev_cls, MAIN_DISPATCHER
from ryu.topology import event, api
from ryu.lib.packet import ethernet, arp, packet, ether_types

import networkx as nx
import time

class EventTopologyChanged(EventBase):

    '''
    Constructor
    '''
    def __init__(self, dg):
        super(EventTopologyChanged, self).__init__()
        self.topo = dg

class EventNewHostInTopology(EventBase):

    '''
    Constructor
    '''
    def __init__(self):
        super(EventNewHostInTopology, self).__init__()

class TopologyMonitor(app_manager.RyuApp):
    
    OFP_VERSION = [ofproto_v1_4.OFP_VERSION]

    '''
    Constructor
    '''
    def __init__(self, *args, **kwargs):
        super(TopologyMonitor, self).__init__(*args, **kwargs)
        self.logger.info("Instantiating RyuApp - TopologyMonitor")
        self.topo = nx.DiGraph()

    '''
    Function invoked when switch enters
    '''
    @set_ev_cls(event.EventSwitchEnter, MAIN_DISPATCHER)
    def new_switch(self, ev):
        dpid = ev.switch.dp.id
        self.logger.info("Switch detected. DPID - %d", dpid)
        
        self.topo.add_node(dpid)
        self.send_event_to_observers(EventTopologyChanged(self.topo))

    '''
    Function invoked when switch leaves
    '''
    @set_ev_cls(event.EventSwitchLeave, MAIN_DISPATCHER)
    def switch_remove(self, ev):
        dpid = ev.switch.dp.id
        self.logger.info("Switch left. DPID - %d", dpid)
        
        self.topo.remove_node(dpid)
        self.send_event_to_observers(EventTopologyChanged(self.topo))


    '''
    Function that listens to link up events
    '''
    @set_ev_cls(event.EventLinkAdd, MAIN_DISPATCHER)
    def link_up(self, ev):
        link = ev.link
        src = link.src
        dst = link.dst
        self.logger.info("Link up. Src - %s, Dst - %s", src, dst)
        
        self.topo.add_edge(src.dpid, dst.dpid, ports=(src.port_no,dst.port_no))
        self.send_event_to_observers(EventTopologyChanged(self.topo))
   
    '''
    Function that listens to link down events
    '''
    @set_ev_cls(event.EventLinkDelete, MAIN_DISPATCHER)
    def link_down(self, ev):
        link = ev.link
        self.logger.info("Link down, Src - %s, Dst - %s", link.src, link.dst)

        self.topo.remove_edge(src.dpid, dst.dpid)
        self.send_event_to_observers(EventTopologyChanged(self.topo))

    '''
    Function that listens to host events
    '''
    @set_ev_cls(event.EventHostAdd, MAIN_DISPATCHER)
    def new_host(self, ev):
        
        host_mac = ev.host.mac
        host_switch = ev.host.port.dpid
        host_port = ev.host.port.port_no

        self.topo.add_edge(host_mac, host_switch, ports=(0, host_port))
        self.topo.add_edge(host_switch, host_mac, ports=(host_port, 0))
        self.send_event_to_observers(EventNewHostInTopology())
        self.send_event_to_observers(EventTopologyChanged(self.topo))

        self.logger.info("New Host - %s at switch %s, port %s", host_mac, host_switch, host_port)
