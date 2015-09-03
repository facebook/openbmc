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


import json
import os
import re

# Handler for sensors resource endpoint
def get_sensors():
    result = []
    data = os.popen('sensors').read()
    data = re.sub(r'\(.+?\)', '', data)
    for edata in data.split('\n\n'):
        adata = edata.split('\n', 1)
        sresult = {}
        if (len(adata) < 2):
            break;
        sresult['name'] = adata[0]
        for sdata in adata[1].split('\n'):
            tdata = sdata.split(':')
            if (len(tdata) < 2):
                continue
            sresult[tdata[0].strip()] = tdata[1].strip()
        result.append(sresult)
    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }
    return fresult
