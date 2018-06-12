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


import os
from subprocess import *
from node import node
from pal import *

class configNode(node):
    def __init__(self, name = None, actions = None):
        self.name = name

        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        result = {}
        cmd = '/usr/local/bin/cfg-util dump-all'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        sdata = data.split('\n');
        if self.name.find('slot') != -1:
            altname = self.name.replace('slot', 'server')
        elif self.name.find('server') != -1:
            altname = self.name.replace('server', 'slot')

        for line in sdata:
            # skip lines that does not start with name
            if line.find(self.name) != -1 or line.find(altname) != -1:
                kv = line.split(':')
                result[kv[0].strip()] = kv[1].strip()
        return result

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            res = "failure"
            # Get the list of parameters to be updated
            params = data["update"]
            if self.name.find('slot') != -1:
                altname = self.name.replace('slot', 'server')
            elif self.name.find('server') != -1:
                altname = self.name.replace('server', 'slot')

            for key in list(params.keys()):
                # update only if the key starts with the name
                if key.find(self.name) != -1 or key.find(altname) != -1:
                    ret = pal_set_key_value(key, params[key])
                    if ret.startswith('Usage'):
                        res = "failure"
                    elif ret == "":
                        res = "success"
                    else:
                        res = ret.strip()

            result = {"result": res}

        return result

def get_node_config(name, is_read_only=True):
    if is_read_only:
        actions = []
    else:
        actions = ["update"]
    return configNode(name = name, actions = actions)
