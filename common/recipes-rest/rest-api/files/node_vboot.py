#!/usr/bin/env python
#
# Copyright 2017-present Facebook. All Rights Reserved.
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
import re
from node import node
from uuid import getnode as get_mac

class vbootNode(node):
    def __init__(self, info = None, actions = None):
        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self):
        data = Popen('/usr/local/bin/vboot-util', \
                shell=True, stdout=PIPE).stdout.read().decode().splitlines()
        info = dict()
        info["status"] = data[-1]
        error=data[-2]
        m = re.match("Status type \((\d+)\) code \((\d+)\)", error)
        if m:
            info["error"] = m.group(1) + "." + m.group(2)
        for l in data:
            a = l.split(": ")
            if len(a) > 1:
                key=a[0]
                value=a[1].strip()
                info[key] = value
        return info

def get_node_vboot():
    return vbootNode()
