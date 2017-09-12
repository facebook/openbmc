#!/usr/bin/env python

from node import node
from pal import *

def get_node_peb():
    name = pal_get_platform_name()
    location = Popen('cat /tmp/tray_location', \
                     shell=True, stdout=PIPE).stdout.read().strip('\n')
    data = Popen('cat /sys/class/gpio/gpio108/value', \
                   shell=True, stdout=PIPE).stdout.read().strip('\n')
    if data == "0":
        status = "In"
    elif data == "1":
        status = "Out"
    else:
        status = "Unknown"

    info = {
            "Description": name + " PCIe Expansion Board",
            "Tray Location": location,
            "Tray Status" : status,
    }

    return node(info)
