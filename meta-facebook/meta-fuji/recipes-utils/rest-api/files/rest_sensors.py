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
import syslog

from rest_utils import DEFAULT_TIMEOUT_SEC


#
# "SENSORS" REST API handler for Fuji
#
# Fuji sensor REST API handler is unique in that,
# it uses rest-api-1, but still uses sensor-util, instead of sensor.
# We are forced to use sensor-util, as Fuji has too many resources
# for BMC to control. If we use conventional way of fetching sensor data
# (sensors command), it will take too long. So we use sensor-util, which
# will use the cached value. But the output format of sensor-util is quite
# different from sensors command. So we need a separate REST api handler
# for this.
#
def get_fru_sensor(fru, removeNA=False):
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

    result["name"] = fru
    result["Adapter"] = fru
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
            # Some of FB Infra services expect "0" instead of NA,
            # even when the PSU is not available or not responding.
            if removeNA:
                # Special case; replace NA with 0
                result[key] = "0"
            else:
                # Regular case
                result[key] = "NA"
    return result


def get_scm_sensors():
    return {"Information": get_fru_sensor("scm"), "Actions": [], "Resources": []}


def get_smb_sensors():
    return {"Information": get_fru_sensor("smb"), "Actions": [], "Resources": []}


def get_psu1_sensors():
    return {
        "Information": get_fru_sensor("psu1", removeNA=True),
        "Actions": [],
        "Resources": [],
    }


def get_psu2_sensors():
    return {
        "Information": get_fru_sensor("psu2", removeNA=True),
        "Actions": [],
        "Resources": [],
    }


def get_psu3_sensors():
    return {
        "Information": get_fru_sensor("psu3", removeNA=True),
        "Actions": [],
        "Resources": [],
    }


def get_psu4_sensors():
    return {
        "Information": get_fru_sensor("psu4", removeNA=True),
        "Actions": [],
        "Resources": [],
    }


def get_pim1_sensors():
    return {"Information": get_fru_sensor("pim1"), "Actions": [], "Resources": []}


def get_pim2_sensors():
    return {"Information": get_fru_sensor("pim2"), "Actions": [], "Resources": []}


def get_pim3_sensors():
    return {"Information": get_fru_sensor("pim3"), "Actions": [], "Resources": []}


def get_pim4_sensors():
    return {"Information": get_fru_sensor("pim4"), "Actions": [], "Resources": []}


def get_pim5_sensors():
    return {"Information": get_fru_sensor("pim5"), "Actions": [], "Resources": []}


def get_pim6_sensors():
    return {"Information": get_fru_sensor("pim6"), "Actions": [], "Resources": []}


def get_pim7_sensors():
    return {"Information": get_fru_sensor("pim7"), "Actions": [], "Resources": []}


def get_pim8_sensors():
    return {"Information": get_fru_sensor("pim8"), "Actions": [], "Resources": []}


def get_aggregate_sensors():
    return {"Information": get_fru_sensor("aggregate"), "Actions": [], "Resources": []}


def get_all_sensors():
    result = []
    # FRUs of /usr/local/bin/sensor-util commands
    frus = [
        "scm",
        "smb",
        "psu1",
        "psu2",
        "psu3",
        "psu4",
        "pim1",
        "pim2",
        "pim3",
        "pim4",
        "pim5",
        "pim6",
        "pim7",
        "pim8",
        "aggregate",
    ]

    for fru in frus:
        sresult = get_fru_sensor(fru, removeNA=True if fru.startswith("psu") else False)
        result.append(sresult)

    fresult = {"Information": result, "Actions": [], "Resources": frus}
    return fresult


def get_sensors():
    return get_all_sensors()
