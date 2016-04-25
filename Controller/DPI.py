'''
Ryu Script for data plane interactions in TSSDN

Naresh Nayak
11.02.2016
'''

from ryu.base.app_manager import RyuApp
from TopologyMonitor import EventTopologyChanged, EventNewHostInTopology
from ryu.topology import event, api
from ryu.controller.handler import set_ev_cls, MAIN_DISPATCHER
from ryu.ofproto import ofproto_v1_4
import networkx as nx
import time

class DPI(RyuApp):

    OFP_VERSION = [ofproto_v1_4.OFP_VERSION]
    
    # Constructor
    def __init__(self, *args, **kwargs):
        super(DPI, self).__init__(*args, **kwargs)
        self.ip_to_mac = {}


    # Set an observer for topology changes
    @set_ev_cls(EventNewHostInTopology, MAIN_DISPATCHER)
    def create_ip_mac_dict(self, ev): 
        
        # Wait for one second for IP address to get updated
        time.sleep(1)
        hosts = api.get_all_host(self)
        if len(self.ip_to_mac) == hosts:
            return
        else:
            for f in hosts:
                if f.ipv4[0]: self.ip_to_mac[f.ipv4[0]]=f.mac

            print self.ip_to_mac

