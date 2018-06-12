#!/usr/bin/env python

from node import node
from pal import *

class fansNode(node):
    def __init__(self, name = None, info = None, actions = None):
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
        result = {}
        cmd = '/usr/local/bin/fan-util --get'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        sdata = data.split('\n')
        for line in sdata:
            # skip lines with " or startin with FRU
            if line.find("Error") != -1:
                continue

            kv = line.split(':')
            if (len(kv) < 2):
                continue

            result[kv[0].strip()] = kv[1].strip()

        return result

def get_node_fans():
    return fansNode()
