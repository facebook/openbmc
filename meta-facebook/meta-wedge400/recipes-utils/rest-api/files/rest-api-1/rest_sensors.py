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

import json
import re
import subprocess
import syslog

from rest_utils import DEFAULT_TIMEOUT_SEC


#
# "SENSORS" REST API handler for Wedge400
#
# Wedge400 sensor REST API handler is unique in that,
# it uses rest-api-1, but still uses sensor-util, instead of sensor.
# We are forced to use sensor-util, as Wedge400 has too many resources
# for BMC to control. If we use conventional way of fetching sensor data
# (sensors command), it will take too long. So we use sensor-util, which
# will use the cached value. But the output format of sensor-util is quite
# different from sensors command. So we need a separate REST api handler
# for this.
#
def get_fru_sensor(fru):
    result = {}
    cmd = "/usr/local/bin/sensor-util"
    proc = subprocess.Popen([cmd, fru], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
        data = data.decode()
    except proc.TimeoutError as ex:
        data = ex.output
        data = data.decode()
        proc.kill()
        syslog.syslog(syslog.LOG_CRIT, "rest_sensor fru {} timeout".format(fru))
        result["result"] = False
        result["reason"] = "{} {} timeout".format(cmd, fru)

    # We flatten all key value pair into one level
    # We keep the JSON format compatible with other
    # FBOSS chassis, to make bmc_proxy happy
    result["name"] = "util"
    result["Adapter"] = "util"
    if "not present" in data:
        result["present"] = False
        return result

    result["present"] = True
    for edata in data.split("\n"):
        # Per each line
        adata = edata.split()
        # For each key value pair
        if len(adata) < 4:
            continue
        key = adata[0].strip()
        value = adata[3].strip()
        try:
            value = float(value)
            result[key] = "{:.2f}".format(value)
        except Exception:
            result[key] = "NA"
    return result


def get_scm_sensors():
    return {"Information": get_fru_sensor("scm"), "Actions": [], "Resources": []}


def get_smb_sensors():
    return {"Information": get_fru_sensor("smb"), "Actions": [], "Resources": []}


def get_pem1_sensors():
    return {"Information": get_fru_sensor("pem1"), "Actions": [], "Resources": []}


def get_pem2_sensors():
    return {"Information": get_fru_sensor("pem2"), "Actions": [], "Resources": []}


def get_psu1_sensors():
    return {"Information": get_fru_sensor("psu1"), "Actions": [], "Resources": []}


def get_psu2_sensors():
    return {"Information": get_fru_sensor("psu2"), "Actions": [], "Resources": []}


def get_all_sensors():
    result = {}
    # FRUs of /usr/local/bin/sensor-util commands
    frus = ["scm", "smb", "psu1", "psu2", "pem1", "pem2"]

    for fru in frus:
        sresult = get_fru_sensor(fru)
        result[fru] = sresult

    fresult = {"Information": result, "Actions": [], "Resources": frus}
    return fresult


def get_sensors():
    return get_all_sensors()
