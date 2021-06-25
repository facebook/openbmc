#!/usr/bin/env python

from node import node
from rest_pal_legacy import pal_get_platform_name


def get_node_e1s_iocm():
    name = pal_get_platform_name()
    info = {"Description": name + " E1.S or IOC Module"}

    return node(info)
