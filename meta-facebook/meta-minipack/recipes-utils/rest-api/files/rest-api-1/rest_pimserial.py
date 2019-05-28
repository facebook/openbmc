#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for getting PIM info

pimserial_re = re.compile("\s*(\S.*) : (\S.*)")


def prepare_pimserial():
    pim_serial = {
        "PIM1": "NA",
        "PIM2": "NA",
        "PIM3": "NA",
        "PIM4": "NA",
        "PIM5": "NA",
        "PIM6": "NA",
        "PIM7": "NA",
        "PIM8": "NA",
    }
    current_pim = 0
    proc = subprocess.Popen(
        ["/usr/local/bin/dump_pim_serials.sh", "-u"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        data, err = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
        for text_line in data.decode().splitlines():
            # Check if this line shows PIM slot number
            m = pimserial_re.match(text_line, 0)
            if m:
                pim_serial[m.group(1)] = m.group(2)
    except Exception as ex:
        pass

    return pim_serial


def get_pimserial():
    result = {}
    pim_serial = prepare_pimserial()
    fresult = {"Information": pim_serial, "Actions": [], "Resources": []}
    return fresult
