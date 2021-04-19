#!/usr/bin/env python

from subprocess import *

from kv import FPERSIST, kv_get
from node import node
from rest_pal_legacy import *



class identifyNode(node):
    def __init__(self, name, info=None, actions=None):
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
        identify_status = kv_get("identify_slot1", FPERSIST)
        info = {"Status of identify LED": identify_status}

        return info

    def doAction(self, data, param={}):
        if data["action"] == "on":
            cmd = "/usr/bin/fpc-util --identify on"
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            if data.startswith("Usage"):
                res = "failure"
            else:
                res = "success"
        elif data["action"] == "off":
            cmd = "/usr/bin/fpc-util --identify off"
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            if data.startswith("Usage"):
                res = "failure"
            else:
                res = "success"
        else:
            res = "not support this action"

        result = {"result": res}

        return result


def get_node_identify(name):
    actions = ["on", "off"]
    return identifyNode(name=name, actions=actions)
