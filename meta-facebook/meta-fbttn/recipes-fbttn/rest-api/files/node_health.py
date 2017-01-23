#!/usr/bin/env python

from subprocess import *
from node import node
from pal import *

class healthNode(node):
    def __init__(self, name = None, info = None, actions = None):
        self.name = name

        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self):
    
        # Enclosure health LED status (GOOD/BAD)
        dpb_hlth = Popen('cat /mnt/data/kv_store/dpb_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        iom_hlth = Popen('cat /mnt/data/kv_store/iom_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        nic_hlth = Popen('cat /mnt/data/kv_store/nic_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        scc_hlth = Popen('cat /mnt/data/kv_store/scc_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        slot1_hlth = Popen('cat /mnt/data/kv_store/slot1_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()

        if ((dpb_hlth == "1")&(iom_hlth == "1")&(nic_hlth == "1")&(scc_hlth == "1")&(slot1_hlth == "1")):
           result="Good"
        else:
           result="Bad"
        
        info = {
           "Status of enclosure health LED": result
        }

        return info
    
def get_node_health():
    return healthNode()
