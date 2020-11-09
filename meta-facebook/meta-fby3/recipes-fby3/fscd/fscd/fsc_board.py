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

dimm_location_name_map = {
    "a": "_dimm0_location",
    "b": "_dimm2_location",
    "c": "_dimm4_location",
    "d": "_dimm6_location",
    "e": "_dimm8_location",
    "f": "_dimm10_location",
}

# For now, it only supports 1OU
m2_1ou_name_map = {
    "a": "_m2_0_info",
    "b": "_m2_1_info",
    "c": "_m2_2_info",
    "d": "_m2_3_info",
}

host_ready_map = {
    "slot1": "fru1_host_ready",
    "slot2": "fru2_host_ready",
    "slot3": "fru3_host_ready",
    "slot4": "fru4_host_ready",
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


def is_dev_prsnt(filename):
    dev_prsnt = 0
    if not os.path.exists(filename):
        return False

    # read byte1
    with open(filename, "rb") as f:
        dev_prsnt = f.read(1)

    if dev_prsnt == b'\x01':
        return True

    return False

def sensor_valid_check(board, sname, check_name, attribute):
    status = c_uint8(0)
    is_valid_check = False
    try:
        if attribute["type"] == "power_status":
            lpal_hndl.pal_get_server_power(int(fru_map[board]["slot_num"]), byref(status))
            if (status.value == 1): # power on
                if match(r"soc_cpu|soc_therm", sname) is not None:
                    is_valid_check = True
                elif match(r"spe_ssd", sname) is not None:
                    # get SSD present status
                    cmd = '/usr/bin/bic-util slot1 0xe0 0x2 0x9c 0x9c 0x0 0x15 0xe0 0x34 0x9c 0x9c 0x0 0x0 0x3'
                    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
                    response = response.decode()
                    # check the completion code
                    if response.split(' ')[6] != '00':
                        return 0
                    prsnt_bits = response.split(' ')[-3]
                    int_val = int('0x' + prsnt_bits, 16)
                    ssd_id = int(sname[7])
                    if int_val & (1 << ssd_id):
                        return 1
                    else:
                        return 0
                else:
                    suffix = ""
                    if  match(r"1ou_m2", sname) is not None:
                        # 1ou_m2_a_temp. key is at 7
                        suffix = m2_1ou_name_map[sname[7]]
                    elif match(r"soc_dimm", sname) is not None:
                        # soc_dimma_temp. key is at 8
                        suffix = dimm_location_name_map[sname[8]]

                    file = "/mnt/data/kv_store/sys_config/" + fru_map[board]["name"] + suffix
                    if is_dev_prsnt(file) == True:
                        is_valid_check = True

            if is_valid_check == True:
                # Check power status again
                file = "/tmp/cache_store/" + host_ready_map[board]
                if not os.path.exists(file):
                   return 0
                with open(file,"r") as f:
                    flag_status = f.read()
                if (flag_status == "1"):
                    return 1

        return 0
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.warn("Exception with board=%s, sensor_name=%s" % (board, sname))
    return 0
