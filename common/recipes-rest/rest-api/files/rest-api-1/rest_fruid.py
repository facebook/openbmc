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

import json
import re
import subprocess
import bmc_command

# Handler for FRUID resource endpoint
def get_fruid(cmd=['weutil']):
    result = {}
    proc = subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    # need to remove the first info line from weutil
    adata = data.split('\n', 1)
    for sdata in adata[1].split('\n'):
        tdata = sdata.split(':', 1)
        if (len(tdata) < 2):
            continue
        result[tdata[0].strip()] = tdata[1].strip()
    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }
    return fresult
