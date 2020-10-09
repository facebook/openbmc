#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
from typing import Dict


PATH = "/tmp/sup_weutil.txt"


# Handler for seutil resource endpoint
def get_seutil() -> Dict:
    return {"Information": get_seutil_data(), "Actions": [], "Resources": []}


def get_seutil_data() -> Dict:
    result = {}
    if not os.path.exists(PATH):
        raise Exception("Path for sup_weutil doesn't exist")
    with open(PATH, "r") as fp:
        # start reading after lines 8
        lines = fp.readlines()[8:]
        if lines:
            for line in lines:
                tdata = line.split(":", 1)
                if len(tdata) < 2:
                    continue
                result[tdata[0].strip()] = tdata[1].strip()
        else:
            raise Exception("sup_weutil file is empty")
        return result
