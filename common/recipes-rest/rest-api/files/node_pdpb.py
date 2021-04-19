#!/usr/bin/env python

from node import node
from rest_pal_legacy import *


def get_node_pdpb():
    name = pal_get_platform_name()
    info = {"Description": name + " PCIe Drive Plane Board"}

    return node(info)
