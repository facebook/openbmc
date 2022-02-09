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
import subprocess
from asyncio import TimeoutError

from common_utils import async_exec
from node import node
from rest_pal_legacy import pal_set_key_value


class configNode(node):
    def __init__(self, name=None, actions=None):
        self.name = name
        self.altname = name
        # Some times we use server1-4 instead of slot1-4
        # So we should look for keys named slot* or server*
        if name.find("slot") != -1:
            self.altname = name.replace("slot", "server")
        elif name.find("server") != -1:
            self.altname = name.replace("server", "slot")
        self.actions = actions

    async def getInformation(self, param={}):
        result = {}
        cmd = ["/usr/local/bin/cfg-util", "dump-all"]
        try:
            retcode, stdout, err = await async_exec(cmd, shell=False)
            sdata = stdout.split("\n")
            if retcode != 0 or err:
                result = {"status": "failure"}
            else:
                for line in sdata:
                    # skip lines that does not start with name
                    if line.find(self.name) != -1 or line.find(self.altname) != -1:
                        kv = line.split(":")
                        result[kv[0].strip()] = kv[1].strip()
        except (TimeoutError, IndexError):
            result = {"status": "failure"}
        except FileNotFoundError:
            result = {"status": "unsupported"}
        return result

    async def doAction(self, data, param={}):
        res = "failure"
        if "update" not in data:
            return {"result": "parameter-error"}

        # Get the list of parameters to be updated
        params = data["update"]
        for key in list(params.keys()):
            # update only if the key starts with the name
            if key.find(self.name) != -1 or key.find(self.altname) != -1:
                try:
                    await pal_set_key_value(key, params[key])
                    res = "success"
                    continue
                except TimeoutError as e:
                    res = e.output.strip()
                    break
                except ValueError as e:
                    res = str(e).strip()
                    break
            else:
                res = "refused: %s" % key
                break
        if res == "":
            res = "failure"
        result = {"result": res}
        return result


def get_node_config(name):
    actions = ["update"]
    return configNode(name=name, actions=actions)
