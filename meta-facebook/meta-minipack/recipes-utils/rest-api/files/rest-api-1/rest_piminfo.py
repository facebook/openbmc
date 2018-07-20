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

pim_number_re = re.compile('\s*PIM (\S.*):')
fpga_not_found_re = re.compile('\s*DOMFPGA is not detected')
fpga_type_re = re.compile('\s*(\S.*) DOMFPGA:')

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
        m = fpga_type_re.match(text_line, 0)
        if m:
            result['PIM'+str(current_pim)] = m.group(1)
        m = fpga_not_found_re.match(text_line, 0)
        if m:
            result['PIM'+str(current_pim)] = 'NA'
    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }
    return fresult
