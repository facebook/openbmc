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
from rest_pim_present import check_pim_presence

# Handler for getting PIM info
# So far, YAMP has only 16Qs.
pim_number_re = re.compile('\s*PIM (\S.*) : (\S.*)')

def get_piminfo():
    current_pim = 0
    result = {}
    proc = subprocess.Popen(['/usr/local/bin/fpga_ver.sh', '-u'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    except proc.TimeoutError as ex:
        data = ex.output
        err = ex.error

    text_lines = data.split(b'\n', 1)
    for text_line_bytes in data.split(b'\n'):
        text_line=text_line_bytes.decode()
        # Check if this line shows PIM slot number
        m = pim_number_re.match(text_line, 0)
        if m:
            current_pim = m.group(1)
            pim_ver = m.group(2)
            # FPGA returns 0x0 if pim is not present
            # Old YAMP FPGA has a bug that returns
            # wrong pim_version. So, use pim_presence information
            # to make sure we are returning the correct value
            if pim_ver == '0x0' or not check_pim_presence(int(current_pim)):
                pim_type = 'NA'
            else:
                pim_type = '16Q'
            result['PIM'+str(current_pim)] = pim_type
    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }
    return fresult
