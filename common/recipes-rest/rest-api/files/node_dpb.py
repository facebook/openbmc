#!/usr/bin/env python

from node import node
from pal import *


def get_node_dpb():
    name = pal_get_platform_name().decode()
    info = {
        "Description": name + " Drive Plan Board",
    }
    return node(info)
