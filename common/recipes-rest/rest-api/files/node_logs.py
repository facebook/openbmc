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

import re
from subprocess import *
from node import node

class logsNode(node):
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
        linfo = []
        cmd = '/usr/local/bin/log-util ' + self.name +' --print'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        data = data.decode()
        sdata = data.split('\n')
        for line in sdata:
            # skip lines with --- or startin with FRU
            if line.startswith( 'FRU' ):
                continue

            # skip empty lines
            if line in ['\n', '\r\n']:
                continue

            line = line.rstrip()
            cells = re.split("\s{2,}", line, maxsplit=4)
            if (len(cells) != 5):
                continue

            temp = {
                    "FRU#": cells[0],
                    "FRU_NAME": cells[1],
                    "TIME_STAMP": cells[2],
                    "APP_NAME": cells[3],
                    "MESSAGE": cells[4],
                   }

            linfo.append(temp)

        result = { "Logs": linfo }
        return result

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] != "clear":
                res = 'failure'
            else:
                cmd = '/usr/local/bin/log-util ' + self.name +' --clear'
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
                data = data.decode()
                if data.startswith( 'Usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            result = { "result": res }

        return result

def get_node_logs(name, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions = [ "clear" ]
    return logsNode(name = name, actions = actions)
