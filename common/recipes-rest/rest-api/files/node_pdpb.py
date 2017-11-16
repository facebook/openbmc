#!/usr/bin/env python

from node import node
from pal import *

def get_node_pdpb():
    name = pal_get_platform_name().decode()
    info = {
            "Description": name + " PCIe Drive Plane Board",
    }

    return node(info)
