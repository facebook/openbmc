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

import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for seutil resource endpoint
def get_seutil() -> Dict:
    return {"Information": get_seutil_data(), "Actions": [], "Resources": []}


def _parse_seutil_data(data) -> Dict:
    result = {}
    # need to remove the first info line from seutil
    adata = data.split("\n", 1)
    for sdata in adata[1].split("\n"):
        tdata = sdata.split(":", 1)
        if len(tdata) < 2:
            continue
        result[tdata[0].strip()] = tdata[1].strip()
    return result


def get_seutil_data() -> Dict:
    cmd = ["/usr/local/bin/seutil"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_seutil_data(data)
