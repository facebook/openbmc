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
from shlex import quote
from subprocess import *

from common_utils import async_exec
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

    async def getInformation(self, param={}):
        linfo = []
        cmd = "/usr/local/bin/log-util " + quote(self.name) + " --print --json"
        _, stdout, _ = await async_exec(cmd, shell=True)
        return json.loads(stdout)

    async def doAction(self, data, param={}):
        if data["action"] != "clear":
            res = "failure"
        else:
            cmd = "/usr/local/bin/log-util " + quote(self.name) + " --clear"
            _, stdout, _ = await async_exec(cmd, shell=True)
            if stdout.startswith("Usage"):
                res = "failure"
            else:
                res = "success"
        result = {"result": res}

        return result


def get_node_logs(name):
    actions = ["clear"]
    return logsNode(name=name, actions=actions)
