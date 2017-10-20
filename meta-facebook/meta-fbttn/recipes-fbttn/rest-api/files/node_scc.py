#!/usr/bin/env python

from node import node
from pal import *

def get_node_scc():
    name = pal_get_platform_name().decode()
    info = {
            "Description": name + " Storage Controller Card",
    }

    return node(info)
