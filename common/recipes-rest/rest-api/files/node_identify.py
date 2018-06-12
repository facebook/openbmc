#!/usr/bin/env python

from subprocess import *
from node import node
from pal import *

class identifyNode(node):
    def __init__(self, name, info = None, actions = None):
        self.name = name

        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        identify_status=""
        data = Popen('cat /mnt/data/kv_store/identify_slot1', \
                            shell=True, stdout=PIPE).stdout.read().decode()
        identify_status = data.strip('\n')
        info = {
                "Status of identify LED": identify_status
        }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "on":
                cmd = '/usr/bin/fpc-util --identify on'
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
                if data.startswith( 'Usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            elif data["action"] == "off":
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

def get_node_identify(name, is_read_only=True):
    if is_read_only:
        actions = []
    else:
        actions = [ "on", "off" ]
    return identifyNode(name = name, actions = actions)
