#!/usr/bin/env python

from kv import kv_get
from node import node
from pal import *


class flashNode(node):
    def __init__(self, name=None, info=None, actions=None):
        self.name = name

        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        # Get vendor name
        vendor_name = " "
        data = kv_get("ssd_vendor")
        vendor_name = data.strip("\n")

        # Get flash type
        flash_type = " "
        data = kv_get("ssd_sku_info")
        flash_type = data.strip("\n")

        info = {"flash type": flash_type, "vendor name": vendor_name}

        return info


def get_node_flash():
    return flashNode()
