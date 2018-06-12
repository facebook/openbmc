#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

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

    def getInformation(self, param={}):
        result = {}
        cmd = '/usr/local/bin/sensor-util ' + self.name + ' --threshold'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        sdata = data.split('\n')
        for line in sdata:
            # skip lines with " or startin with FRU
            if line.find("bic_read_sensor_wrapper") != -1:
                continue
            if line.find("failed") != -1:
                continue
            if line.find(":") == -1:
                continue

            kv = [line[0:line.find(':')-1], line[line.find(':')+1:]]
            if (len(kv) < 2):
                continue

            result[kv[0].strip()] = kv[1].strip()

        return result

def get_node_sensors(name):
    return sensorsNode(name)
