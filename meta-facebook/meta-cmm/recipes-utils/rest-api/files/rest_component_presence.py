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

import re
import subprocess


def get_presence():
    result = {}
    p = subprocess.Popen(
        ["/usr/local/bin/presence_util.sh"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    data, err = p.communicate()
    data = data.decode()
    rc = p.returncode

    if rc < 0:
        result = " failed with returncode = " + str(rc) + " and error " + err
    else:
        data = re.sub("\(.+?\)", "", data)
        for edata in data.split("\n"):
            tdata = edata.split(":")
            if len(tdata) < 2:
                continue
            result[tdata[0].strip()] = tdata[1].strip()
    fresult = {"Information": result, "Actions": [], "Resources": []}
    return fresult
