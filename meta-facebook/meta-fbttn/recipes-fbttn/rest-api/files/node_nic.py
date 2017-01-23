#!/usr/bin/env python

from node import node
from pal import *

def get_node_nic():
    name = pal_get_platform_name()
    info = {
            "Description": name + " Mezzanine Card",
    }

    return node(info)