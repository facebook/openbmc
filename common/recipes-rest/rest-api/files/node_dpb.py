#!/usr/bin/env python

from node import node
from rest_pal_legacy import *


def get_node_dpb():
    name = pal_get_platform_name()
    info = {"Description": name + " Drive Plan Board"}
    return node(info)
