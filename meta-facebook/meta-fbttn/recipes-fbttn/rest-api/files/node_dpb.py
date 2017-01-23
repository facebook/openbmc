#!/usr/bin/env python

from node import node
from pal import *

class dpbNode(node):
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

    def getInformation(self):
        result = {}

        name = pal_get_platform_name()
        result["Description"] = name + " Drive Plan Board"

        cmd = '/usr/bin/enclosure-util --error'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        data = data.strip()
        sdata = data.split('\n')
        for line in sdata:
            kv = line.split(':')
            result[kv[0].strip()] = kv[1].strip()

        cmd = '/usr/bin/enclosure-util --hdd-status'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        data = data.strip()
        sdata = data.split('\n')
        for line in sdata:
            kv = line.split(':')
            if (len(kv) < 2):
               continue
            result[kv[0].strip()] = kv[1].strip()

        for hddnum in range(0, 35):
            cmd = '/usr/bin/enclosure-util --hdd-status '+ str(hddnum)
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
            data = data.strip()
            kv = data.split(':')
            result[kv[0].strip()] = kv[1].strip()

        return result



def get_node_dpb():
    return dpbNode()
