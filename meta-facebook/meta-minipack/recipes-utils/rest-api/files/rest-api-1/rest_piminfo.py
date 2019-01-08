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
fpga_type_re = re.compile('\s*(\S.*) DOMFPGA: (\S.*)')

def prepare_piminfo():
    pim_type = {str(i): 'NA' for i in range(1, 9)}
    pim_fpga_ver = {str(i): 'NA' for i in range(1, 9)}
    current_pim = 0
    try:
        stdout = subprocess.check_output(
            ['/usr/local/bin/fpga_ver.sh', '-u'], timeout=DEFAULT_TIMEOUT_SEC
        )
        for text_line in stdout.decode().splitlines():
            # Check if this line shows PIM slot number
            m = pim_number_re.match(text_line, 0)
            if m:
                current_pim = m.group(1)
            m = fpga_type_re.match(text_line, 0)
            if m:
                pim_type[current_pim] = m.group(1)
                pim_fpga_ver[current_pim] = m.group(2)
    except Exception as ex:
            pass

    return pim_type, pim_fpga_ver

def prepare_pimver():
    pim_ver = {str(i): 'NA' for i in range(1, 9)}
    for pim in pim_ver:
        stdout = subprocess.check_output(
            ['/usr/local/bin/peutil', pim], timeout=DEFAULT_TIMEOUT_SEC
        )
        for line in stdout.decode().splitlines():
            if 'Version' in line:
                _, version = line.split(':')
                pim_ver[pim] = version.strip()
    return pim_ver

def get_piminfo():
    result = {}
    pim_type, pim_fpga_ver = prepare_piminfo()
    pim_ver = prepare_pimver()
    for pim_number in range(1,9):
            result['PIM'+str(pim_number)] = {
                    "type": pim_type[str(pim_number)],
                    "fpga_ver": pim_fpga_ver[str(pim_number)],
                    "ver": pim_ver[str(pim_number)],
                 }
    fresult = {
        "Information": result,
        "Actions": [],
        "Resources": [],
      }
    return fresult
