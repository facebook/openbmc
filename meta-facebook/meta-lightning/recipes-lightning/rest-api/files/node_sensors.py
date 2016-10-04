#!/usr/bin/env python

from subprocess import *
import json
import os
import re
from node import node

class sensorsNode(node):
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

    def getInformation(self):
        result = {}
        cmd = '/usr/local/bin/sensor-util ' + self.name
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        sdata = data.split('\n')
        for line in sdata:
            # skip lines with " or startin with FRU
            if line.find("failed") != -1:
                continue

            kv = line.split(':')
            if (len(kv) < 2):
                continue

            result[kv[0].strip()] = kv[1].strip()

        return result

def get_node_sensors(name):
    return sensorsNode(name)
