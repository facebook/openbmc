#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for gb frequency resource endpoint
def get_gb_freq():
    try:
        cmd = [
            "source /usr/local/bin/openbmc-utils.sh && cat $PWRCPLD_SYSFS_DIR/gb_freq"
        ]
        proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True
        )
        data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
        if not proc.returncode:
            gb_freq = data.decode(errors="ignore").strip()
            return {"result": gb_freq}
        else:
            return {"result": "failed", "reason": "command {} failed".format(cmd)}
    except (FileNotFoundError, AttributeError) as e:
        return {"result": "failed", "reason": str(e)}
