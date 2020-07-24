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


# Handler for feutil resource endpoint
def get_peutil():
    return {
        "Information": [],
        "Actions": [],
        "Resources": ["pim1", "pim2", "pim3", "pim4", "pim5", "pim6", "pim7", "pim8"],
    }


def _parse_peutil_data(data):
    result = {}
    for eeprom in data:
        sresult = {}
        # the first info line from peutil is eeprom device
        adata = eeprom.split("\n", 1)
        if len(adata) < 2:
            continue
        for sdata in adata[1].split("\n"):
            tdata = sdata.split(":", 1)
            if len(tdata) < 2:
                if len(tdata[0].strip()) > 0:
                    sresult = tdata[0].strip()
                continue
            if len(tdata[1].strip()) > 0:
                sresult[tdata[0].strip()] = tdata[1].strip()
        result[adata[0].strip().strip(":")] = sresult
    return result


def _packet_peutil_date(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    data = data.split("\r\n")
    return {"Information": _parse_peutil_data(data), "Actions": [], "Resources": []}


def get_peutil_pim1_data():
    cmd = ["/usr/local/bin/peutil", "1"]
    return _packet_peutil_date(cmd)


def get_peutil_pim2_data():
    cmd = ["/usr/local/bin/peutil", "2"]
    return _packet_peutil_date(cmd)


def get_peutil_pim3_data():
    cmd = ["/usr/local/bin/peutil", "3"]
    return _packet_peutil_date(cmd)


def get_peutil_pim4_data():
    cmd = ["/usr/local/bin/peutil", "4"]
    return _packet_peutil_date(cmd)


def get_peutil_pim5_data():
    cmd = ["/usr/local/bin/peutil", "5"]
    return _packet_peutil_date(cmd)


def get_peutil_pim6_data():
    cmd = ["/usr/local/bin/peutil", "6"]
    return _packet_peutil_date(cmd)


def get_peutil_pim7_data():
    cmd = ["/usr/local/bin/peutil", "7"]
    return _packet_peutil_date(cmd)


def get_peutil_pim8_data():
    cmd = ["/usr/local/bin/peutil", "8"]
    return _packet_peutil_date(cmd)
