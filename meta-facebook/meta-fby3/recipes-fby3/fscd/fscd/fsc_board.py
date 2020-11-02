#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
from ctypes import CDLL, c_uint8, byref
from re import match
from subprocess import PIPE, Popen

from fsc_util import Logger

import re
import os

lpal_hndl = CDLL("libpal.so.0")

fru_map = {
    "slot1": {
        "name": "fru1",
        "slot_num": 1,
    },
    "slot2": {
        "name": "fru2",
        "slot_num": 2,
    },
    "slot3": {
        "name": "fru3",
        "slot_num": 3,
    },
    "slot4": {
        "name": "fru4",
        "slot_num": 4,
    },
}

dimm_part_name_map = {
    "a": "_dimm0_part_name",
    "b": "_dimm2_part_name",
    "c": "_dimm4_part_name",
    "d": "_dimm6_part_name",
    "e": "_dimm8_part_name",
    "f": "_dimm10_part_name",
}

m2_part_name_map = {
    "a": "_m2_0_info",
    "b": "_m2_1_info",
    "c": "_m2_2_info",
    "d": "_m2_3_info",
}

def board_fan_actions(fan, action="None"):
    """
    Override the method to define fan specific actions like:
    - handling dead fan
    - handling fan led
    """
    if action in "dead":
        # TODO: Why not do return pal_fan_dead_handle(fan)
        lpal_hndl.pal_fan_dead_handle(fan.fan_num)
    elif action in "recover":
        # TODO: Why not do return pal_fan_recovered_handle(fan)
        lpal_hndl.pal_fan_recovered_handle(fan.fan_num)
    elif "led" in action:
        # No Action Needed. LED controlled in action 'dead' and 'recover'.
        pass
    else:
        Logger.warn("%s needs action %s" % (fan.label, str(action)))
    pass


def board_host_actions(action="None", cause="None"):
    """
    Override the method to define fan specific actions like:
    - handling host power off
    - alarming/syslogging criticals
    """
    if "host_shutdown" in action:
        if "All fans are bad" in cause:
            lpal_hndl.pal_fan_fail_otp_check()
    pass


def board_callout(callout="None", **kwargs):
    """
    Override this method for defining board specific callouts:
    - Exmaple chassis intrusion
    """
    if "init_fans" in callout:
        boost = 100  # define a boost for the platform or respect fscd override
        if "boost" in kwargs:
            boost = kwargs["boost"]
        return set_all_pwm(boost)
    else:
        Logger.warning("Callout %s not handled" % callout)
    pass


def set_all_pwm(boost):
    cmd = "/usr/local/bin/fan-util --set %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    response = response.decode()
    return response


def sensor_valid_check(board, sname, check_name, attribute):
    status = c_uint8(0)
    try:
        if attribute["type"] == "power_status":
            lpal_hndl.pal_get_server_power(int(fru_map[board]["slot_num"]), byref(status))
            if (status.value == 1): # power on
                if match(r"soc_cpu", sname) is not None:
                    ret = 1
                elif match(r"soc_therm", sname) is not None:
                    ret = 1
                elif match(r"1ou_m2", sname) is not None:
                    # check m2 present
                    file = "/mnt/data/kv_store/sys_config/" + fru_map[board]["name"] + part_name_map[sname[8]]
                    if not os.path.exists(file):
                        return 0
                    with open(file, "r") as f:
                        m2_prsnt = f.read(1)
                    if (m2_prsnt == 0x01): # present
                        ret = 1
                    else:
                        ret = 0
                elif match(r"soc_dimm", sname) is not None:
                    # check DIMM present
                    file = "/mnt/data/kv_store/sys_config/" + fru_map[board]["name"] + dimm_part_name_map[sname[8]]
                    if not os.path.exists(file):
                        return 0
                    with open(file, "r") as f:
                        dimm_sts = f.readline()
                    if re.search(r"([a-zA-Z0-9])", dimm_sts):
                        ret = 1
                    else:
                        ret = 0
                # Check power status again
                # Sync with the pwr_off_flag global value in pal_sensor
                if (lpal_hndl.pwr_off_flag[int(fru_map[board]["slot_num"])-1] == 0):
                    return 1
                else:
                    return ret
            else:
                return 0
        return 0
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.warn("Exception with board=%s, sensor_name=%s" % (board, sname))
    return 0
