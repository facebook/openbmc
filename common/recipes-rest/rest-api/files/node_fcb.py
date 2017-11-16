#!/usr/bin/env python

from node import node
from pal import *

def get_node_fcb():
    name = pal_get_platform_name().decode()
    info = {
            "Description": name + " Fan Control Board",
           }

    return node(info)
