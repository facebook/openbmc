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
        peb_hlth = Popen('cat /tmp/peb_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        pdpb_hlth = Popen('cat /tmp/pdpb_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        fcb_hlth = Popen('cat /tmp/fcb_sensor_health', \
                            shell=True, stdout=PIPE).stdout.read()
        bmc_hlth = Popen('cat /tmp/bmc_health', \
                            shell=True, stdout=PIPE).stdout.read()

        if ((peb_hlth == "1") and (pdpb_hlth == "1")
                and (fcb_hlth == "1") and (bmc_hlth == "1")):
            result="Good"
        else:
            result="Bad"
        
        info = {
           "Status of enclosure health LED": result
        }

        return info
    
def get_node_health():
    return healthNode()
