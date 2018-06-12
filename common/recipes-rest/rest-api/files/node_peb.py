#!/usr/bin/env python

from node import node
from pal import *

class pebNode(node):
    def __init__(self, name = None, info = None, actions = None):
        name = pal_get_platform_name().decode()
        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        name = pal_get_platform_name().decode()
        location = Popen('cat /tmp/tray_location', \
                         shell=True, stdout=PIPE).stdout.read().decode().strip('\n')
        data = Popen('cat /sys/class/gpio/gpio108/value', \
                       shell=True, stdout=PIPE).stdout.read().decode().strip('\n')
        if data == "0":
            status = "In"
        elif data == "1":
            status = "Out"
        else:
            status = "Unknown"

        data = Popen('cat /mnt/data/kv_store/system_identify', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        identify_status = data.strip('\n')

        info = {
                "Description": name + " PCIe Expansion Board",
                "Tray Location": location,
                "Tray Status" : status,
                "Status of identify LED": identify_status,
        }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "identify-on":
                cmd = '/usr/bin/fpc-util --identify on'
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
                if data.startswith( 'Usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            elif data["action"] == "identify-off":
                cmd = '/usr/bin/fpc-util --identify off'
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
                if data.startswith( 'Usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            else:
                res = 'not support this action'

            result = { "result": res }

        return result


def get_node_peb(is_read_only=True):
    if is_read_only:
        actions = []
    else:
        actions = [ "identify-on", "identify-off" ]

    return pebNode(actions = actions)
