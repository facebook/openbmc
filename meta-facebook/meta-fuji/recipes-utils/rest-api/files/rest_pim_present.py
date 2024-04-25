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


def load_pim_state():
    all_pims = {"pim" + str(i): "Removed" for i in range(1, 9)}

    try:
        stdout = subprocess.check_output(
            ["/usr/local/bin/presence_util.sh"], timeout=DEFAULT_TIMEOUT_SEC
        )
        for line in stdout.decode().splitlines():
            if line.startswith("pim"):
                key, value = line.split(":")

                pim_id = key.strip()
                pim_state = int(value.strip())
                if pim_state == 1:
                    all_pims[pim_id] = "Present"
    except Exception:
        pass

    return all_pims


def get_pim_present():
    state = load_pim_state()

    result = {"Information": state, "Actions": [], "Resources": []}
    return result
