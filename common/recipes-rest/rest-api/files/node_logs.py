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
import json
import re
from subprocess import *

from node import node


class logsNode(node):
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
        linfo = []
        cmd = "/usr/local/bin/log-util " + self.name + " --print" + " --json"
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        data = data.decode()
        return json.loads(data)

    def doAction(self, data, param={}):
        if data["action"] != "clear":
            res = "failure"
        else:
            cmd = "/usr/local/bin/log-util " + self.name + " --clear"
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
            data = data.decode()
            if data.startswith("Usage"):
                res = "failure"
            else:
                res = "success"
        result = {"result": res}

        return result


def get_node_logs(name):
    actions = ["clear"]
    return logsNode(name=name, actions=actions)
