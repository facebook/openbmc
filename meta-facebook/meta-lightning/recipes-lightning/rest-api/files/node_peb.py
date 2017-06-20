#!/usr/bin/env python

from node import node
from pal import *

def get_node_peb():
    name = pal_get_platform_name()
    location = Popen('cat /tmp/tray_location', \
                     shell=True, stdout=PIPE).stdout.read().strip('\n')
    info = {
            "Description": name + " PCIe Expansion Board",
            "Tray Location": location,
    }

    return node(info)
