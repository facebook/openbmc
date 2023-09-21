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
from subprocess import PIPE, Popen
import kv
import os

from fsc_util import Logger

fan_mode = {"normal_mode": 0, "trans_mode": 1, "boost_mode": 2, "progressive_mode": 3}
get_fan_mode_scenario_list = ["one_fan_failure", "sensor_hit_UCR"]

lfby35_hndl = CDLL("libfby35_common.so.0")
lpal_hndl = CDLL("libpal.so.0")
lbic_hndl = CDLL("libbic.so.0")

one_fan_fail_tuple = (fan_mode["trans_mode"], 60)  # (mode, pwm)
GPIO_FM_BIOS_POST_CMPLT_BMC_N = 1

dimm_index_start = 8
gl_dimm_index_start = 7

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

gl_dimm_location_name_map = {
    "a": "_dimm0_location",
    "b": "_dimm1_location",
    "c": "_dimm2_location",
    "d": "_dimm3_location",
    "e": "_dimm4_location",
    "f": "_dimm5_location",
    "g": "_dimm6_location",
    "h": "_dimm7_location",
}

hd_dimm_location_name_map = {
    "0": "_dimm0_location",
    "1": "_dimm1_location",
    "2": "_dimm2_location",
    "4": "_dimm4_location",
    "6": "_dimm6_location",
    "7": "_dimm7_location",
    "8": "_dimm8_location",
    "10": "_dimm10_location",
}

dimm_location_name_map = {
    "0": "_dimm0_location",
    "2": "_dimm4_location",
    "3": "_dimm6_location",
    "4": "_dimm8_location",
    "6": "_dimm12_location",
    "7": "_dimm14_location",
}

host_ready_map = {
    "slot1": "fru1_host_ready",
    "slot2": "fru2_host_ready",
    "slot3": "fru3_host_ready",
    "slot4": "fru4_host_ready",
}


def get_system_conf():
    try:
        return kv.kv_get("sled_system_conf", kv.FPERSIST)

    except Exception:
        return ""


system_conf = get_system_conf()
if "HD" in system_conf:
    one_fan_fail_tuple = (fan_mode["boost_mode"], 80)
    get_fan_mode_scenario_list = [
            "one_fan_failure",
            "sensor_hit_UCR",
            "sensor_fail",
        ]
    GPIO_FM_BIOS_POST_CMPLT_BMC_N = 0
    dimm_location_name_map = hd_dimm_location_name_map
elif "GL" in system_conf:
    dimm_location_name_map = gl_dimm_location_name_map
    dimm_index_start = gl_dimm_index_start
elif "VF" in system_conf:
    for i in [1, 2, 3, 4]:
        lfby35_hndl.fby35_common_get_1ou_m2_prsnt(int(i))
else:
    pass


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
        Logger.warn("Callout %s not handled" % callout)
    pass


def set_all_pwm(boost):
    cmd = "/usr/local/bin/fan-util --set %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    response = response.decode()
    return response


def is_host_ready(filename):
    try:
        ready = kv.kv_get(filename)
        if ready == "1":
            return 1
        return 0

    except Exception:
        return 0


def is_dimm_prsnt(board, num):
    try:
        filename = "sys_config/" + fru_map[board]["name"] + dimm_location_name_map[num]
        val = kv.kv_get(filename, kv.FPERSIST, True)
        if val[0] == 1:
            return 1
        return 0

    except Exception:
        return 0


def is_e1s_prsnt(board, num):
    try:
        filename = fru_map[board]["name"] + "_1ou_m2_prsnt"
        val = kv.kv_get(filename, 0, True)
        if (val[0] & (1 << int(num))) == 0:
            return 1
        return 0

    except Exception:
        return 0


def sensor_valid_check(board, sname, check_name, attribute):
    try:
        if attribute["type"] == "power_status":
            (board, sname) = sname.split("_", 1)
            file = "/var/run/power-util_%d.lock" % int(fru_map[board]["slot_num"])
            if os.path.exists(file):
                return 0
            if "slot" in board:
                if (
                    lpal_hndl.pal_is_fw_update_ongoing(int(fru_map[board]["slot_num"]))
                    == True
                ):
                    return 0
            status = c_uint8(0)
            ret = lpal_hndl.pal_get_server_power(
                int(fru_map[board]["slot_num"]), byref(status)
            )
            if (ret != 0) or (status.value == 5):  # SERVER_12V_OFF
                return 0

            if "fio_" in sname:
                return 1

            if status.value == 1:  # power on
                ready = is_host_ready(host_ready_map[board])
                if ready != 1:
                    return 0

                if "dimm" in sname:
                    dimm_index_end = sname.index("_t")
                    return is_dimm_prsnt(board, sname[dimm_index_start : dimm_index_end])

                if "vf_e1s" in sname:
                    return is_e1s_prsnt(board, sname[10 : sname.find("_t")])

                return 1

        return 0
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.warn("Exception with board=%s, sensor_name=%s" % (board, sname))
    return 0


def get_fan_mode(scenario="None"):
    if "one_fan_failure" in scenario:
        return one_fan_fail_tuple
    elif "sensor_hit_UCR" in scenario:
        pwm = 100
        return fan_mode["boost_mode"], pwm
    elif "sensor_fail" in scenario:
        pwm = 80
        return fan_mode["boost_mode"], pwm
    pass


def sensor_fail_ignore_check(board, sname):
    if "slot" not in sname:
        return False
    else:
        (board, sname) = sname.split("_", 1)
        slot_id = fru_map[board]["slot_num"]
        pin_val = c_uint8(0)
        lbic_hndl.bic_get_one_gpio_status(
            slot_id, GPIO_FM_BIOS_POST_CMPLT_BMC_N, byref(pin_val)
        )
        if pin_val.value == 0:  # post complete
            return False
        return True
