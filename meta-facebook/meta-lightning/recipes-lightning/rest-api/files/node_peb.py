#!/usr/bin/env python

from node import node
from pal import *

def get_node_peb():
    name = pal_get_platform_name()
    info = {
            "Description": name + " PCIe Expansion Board",
           }

    return node(info)
