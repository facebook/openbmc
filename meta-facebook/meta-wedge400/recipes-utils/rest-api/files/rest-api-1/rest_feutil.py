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


# Handler for feutil resource endpoint
def get_feutil():
    return {
            "Information": [],
            "Actions": [],
            "Resources": ["all", "fcm", "fan1", "fan2", "fan3", "fan4"]
           }

def _parse_feutil_data(data):
    result = {}
    for eeprom in data:
        sresult = {}
        # the first info line from feutil is eeprom device
        adata = eeprom.split('\n', 1)
        if len(adata) < 2:
            continue
        for sdata in adata[1].split('\n'):
            tdata = sdata.split(':', 1)
            if (len(tdata) < 2):
                if len(tdata[0].strip()) > 0:
                    sresult = tdata[0].strip()
                continue
            if len(tdata[1].strip()) > 0:
                sresult[tdata[0].strip()] = tdata[1].strip()
        result[adata[0].strip()] = sresult
    return result

def _packet_feutil_date(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors='ignore')
    data = data.split('\r\n')
    return {
            "Information": _parse_feutil_data(data),
            "Actions": [],
            "Resources": []
           }

def get_feutil_all_data():
    cmd = ["/usr/local/bin/feutil", "all"]
    return _packet_feutil_date(cmd)

def get_feutil_fcm_data():
    cmd = ["/usr/local/bin/feutil", "fcm"]
    return _packet_feutil_date(cmd)

def get_feutil_fan1_data():
    cmd = ["/usr/local/bin/feutil", "1"]
    return _packet_feutil_date(cmd)

def get_feutil_fan2_data():
    cmd = ["/usr/local/bin/feutil", "2"]
    return _packet_feutil_date(cmd)

def get_feutil_fan3_data():
    cmd = ["/usr/local/bin/feutil", "3"]
    return _packet_feutil_date(cmd)

def get_feutil_fan4_data():
    cmd = ["/usr/local/bin/feutil", "4"]
    return _packet_feutil_date(cmd)
