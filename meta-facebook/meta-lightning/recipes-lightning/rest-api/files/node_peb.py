#!/usr/bin/env python

from node import node
from pal import *

class pebNode(node):
    def __init__(self, name = None, info = None, actions = None):
        name = pal_get_platform_name().decode()
        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self):
        name = pal_get_platform_name().decode()
        location = Popen('cat /tmp/tray_location', \
                         shell=True, stdout=PIPE).stdout.read().decode().strip('\n')
        data = Popen('cat /sys/class/gpio/gpio108/value', \
                       shell=True, stdout=PIPE).stdout.read().decode().strip('\n')
        if data == "0":
            status = "In"
        elif data == "1":
            status = "Out"
        else:
            status = "Unknown"

        info = {
                "Description": name + " PCIe Expansion Board",
                "Tray Location": location,
                "Tray Status" : status,
        }

        return info


def get_node_peb():
    return pebNode()

