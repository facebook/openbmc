#!/usr/bin/env python

from node import node
from rest_pal_legacy import *


def get_node_iom():
    name = pal_get_platform_name()

    info = {"Description": name + " IO Module"}

    return node(info)
