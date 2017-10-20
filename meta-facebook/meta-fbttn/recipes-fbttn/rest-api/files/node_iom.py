#!/usr/bin/env python

from node import node
from pal import *

def get_node_iom():
    name = pal_get_platform_name().decode()

    info = {
            "Description": name + " IO Module",
    }

    return node(info)
