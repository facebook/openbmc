#!/usr/bin/env python3
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

# Handler for SOL resource endpoint
def sol_action(data):
    if data["action"] == 'stop':
        (ret, _) = Popen('ps | grep -v grep | grep sol.sh | awk \'{print $1}\'', \
                    shell=True, stdout=PIPE).communicate()
        ret = ret.decode()
        adata = ret.split('\n')
        if adata[0] != '':
            cmd = 'kill -9 ' + adata[0] + ' 2>/dev/null'
            Popen(cmd, \
                         shell=True, stdout=PIPE).communicate()
        (ret, _) = Popen('ps | grep -v grep | grep microcom | awk \'{print $1}\'', \
                    shell=True, stdout=PIPE).communicate()
        ret = ret.decode()
        adata = ret.split('\n')
        if adata[0] != '':
            cmd = 'kill -9 ' + adata[0] + ' 2>/dev/null'
            Popen(cmd, \
                        shell=True, stdout=PIPE).communicate()
        Popen('rm /var/lock/LCK..ttyS1', \
                    shell=True, stdout=PIPE).communicate()

        result = { "result": "success"}
    else:
        result = { "result": "fail"}

    return result
