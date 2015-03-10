#!/usr/bin/env python
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

# Handler for uServer resource endpoint
def get_server():
    ret = Popen('/usr/local/bin/wedge_power.sh status', \
            shell=True, stdout=PIPE).stdout.read()
    status = ret.rsplit()[-1]

    result = {
                "Information": { "status": status },
                "Actions": ["power-on", "power-off", "power-reset"],
                "Resources": [],
             }

    return result

def server_action(data):
    if data["action"] == 'power-on':
        ret = Popen('/usr/local/bin/wedge_power.sh status', \
			        shell=True, stdout=PIPE).stdout.read()
        status = ret.rsplit()[-1]
        if status == 'on':
            res = 'failure'
            reason = 'already on'
        else:
            ret = Popen('/usr/local/bin/wedge_power.sh on', \
			            shell=True, stdout=PIPE).stdout.read()
            res = "success"
    elif data["action"] == 'power-off':
        ret = Popen('/usr/local/bin/wedge_power.sh off', \
			        shell=True, stdout=PIPE).stdout.read()
        res = "success"
    elif data["action"] == 'power-reset':
        ret = Popen('/usr/local/bin/wedge_power.sh reset', \
			        shell=True, stdout=PIPE).stdout.read()
        res = "success"
    else:
        res = 'failure'
        reason = 'invalid action'

    if res == 'failure':
        result = { "result": res, "reason": reason}
    else:
        result = { "result": res }

    return result
