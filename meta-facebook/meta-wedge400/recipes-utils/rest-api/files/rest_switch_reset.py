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
from rest_utils import DEFAULT_TIMEOUT_SEC


def do_switch_reset(cmd):
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
        if not proc.returncode:
            return {"result": "success"}
        else:
            return {"result": "failed", "reason": "command {} failed".format(cmd)}
    except (FileNotFoundError, AttributeError) as e:
        return {"result": "failed", "reason": str(e)}


# Handler for switchreset/cycle_reset resource endpoint
def reset_switch_cycle_reset():
    cmd = ["/usr/local/bin/switch_reset.sh", "cycle"]
    return do_switch_reset(cmd)


# Handler for switchreset/only_reset resource endpoint
def reset_switch_only_reset():
    cmd = ["/usr/local/bin/switch_reset.sh"]
    return do_switch_reset(cmd)


# Handler for switchreset resource endpoint
def reset_switch():
    sub_endpoint = ["cycle_reset", "only_reset"]
    return {"Information": "", "Actions": [], "Resources": sub_endpoint}
