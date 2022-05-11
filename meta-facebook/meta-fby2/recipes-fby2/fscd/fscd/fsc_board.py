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
import re
from ctypes import CDLL, c_char_p
from re import match
from subprocess import PIPE, Popen

import kv
import libgpio
from fsc_common_var import fan_mode
from fsc_util import Logger

lpal_hndl = CDLL("libpal.so.0")
get_fan_mode_scenario_list = []

try:
    with open("/tmp/spb_type") as f:
        spb_type = f.readline()
        if spb_type == "3":  # Northdome
            get_fan_mode_scenario_list = [
                "sensor_hit_UCR",
                "sensor_fail",
                "one_fan_failure",
            ]
except Exception:
    Logger.warn("failed to open or read /tmp/spb_type")

fru_map = {
    "slot1": {
        "name": "fru1",
        "pwr_gpio": "SLOT1_POWER_EN",
        "bic_ready_gpio": "I2C_SLOT1_ALERT_N",
        "slot_num": 1,
        "slot_12v_status": "P12V_STBY_SLOT1_EN",
    },
    "slot2": {
        "name": "fru2",
        "pwr_gpio": "SLOT2_POWER_EN",
        "bic_ready_gpio": "I2C_SLOT2_ALERT_N",
        "slot_num": 2,
        "slot_12v_status": "P12V_STBY_SLOT2_EN",
    },
    "slot3": {
        "name": "fru3",
        "pwr_gpio": "SLOT3_POWER_EN",
        "bic_ready_gpio": "I2C_SLOT3_ALERT_N",
        "slot_num": 3,
        "slot_12v_status": "P12V_STBY_SLOT3_EN",
    },
    "slot4": {
        "name": "fru4",
        "pwr_gpio": "SLOT4_POWER_EN",
        "bic_ready_gpio": "I2C_SLOT4_ALERT_N",
        "slot_num": 4,
        "slot_12v_status": "P12V_STBY_SLOT4_EN",
    },
}

loc_map = {
    "a0": "_dimm0_location",
    "a1": "_dimm1_location",
    "b0": "_dimm2_location",
    "b1": "_dimm3_location",
    "d0": "_dimm4_location",
    "d1": "_dimm5_location",
    "e0": "_dimm6_location",
    "e1": "_dimm7_location",
}

loc_map_nd = {
    "a": "_dimm0_location",
    "c": "_dimm2_location",
    "d": "_dimm3_location",
    "e": "_dimm4_location",
    "g": "_dimm6_location",
    "h": "_dimm7_location",
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


fscd_counter = 0
board_for_counter = ""
sname_for_counter = ""


def all_slots_power_off():
    all_power_off = True
    for board in fru_map:
        with libgpio.GPIO(shadow=fru_map[board]["pwr_gpio"]) as gpio:
            if gpio.get_value() == libgpio.GPIOValue.HIGH:
                all_power_off = False
                break
    return all_power_off


def sensor_fail_ignore_check(board, sname):
    if re.match(r"(.*)host_boot_temp", sname) is not None:
        return True
    return False


def sensor_valid_check(board, sname, check_name, attribute):

    global fscd_counter
    global board_for_counter
    global sname_for_counter

    if not board_for_counter:
        board_for_counter = board
        sname_for_counter = sname

    if str(board) == str(board_for_counter):
        if str(sname) == str(sname_for_counter):
            fscd_counter = lpal_hndl.pal_get_fscd_counter()
            fscd_counter = fscd_counter + 1
            lpal_hndl.pal_set_fscd_counter(fscd_counter)

    if str(board) == "all":
        sdata = sname.split("_")
        board = sdata[0]
        sname = sname.replace(board + "_", "")
    Logger.debug("board=%s sname=%s" % (board, sname))

    try:
        if attribute["type"] == "power_status":
            with libgpio.GPIO(shadow=fru_map[board]["pwr_gpio"]) as gpio:
                if gpio.get_value() == libgpio.GPIOValue.HIGH:
                    slot_id = fru_map[board]["slot_num"]
                    is_slot_server = lpal_hndl.pal_is_slot_support_update(slot_id)
                    if int(is_slot_server) == 1:
                        with libgpio.GPIO(
                            shadow=fru_map[board]["bic_ready_gpio"]
                        ) as bic_gpio:
                            if bic_gpio.get_value() == libgpio.GPIOValue.LOW:
                                if match(r"soc_dimm", sname) is not None:
                                    server_type = lpal_hndl.pal_get_server_type(slot_id)
                                    if int(server_type) == 4:  # ND Server
                                        # check DIMM present
                                        board_name = (
                                            "sys_config/"
                                            + fru_map[board]["name"]
                                            + loc_map_nd[sname[8:9]]
                                        )
                                        dimm_sts = kv.kv_get(
                                            board_name, kv.FPERSIST, binary=True
                                        )
                                    else:  # TL Server
                                        # check DIMM present
                                        board_name = (
                                            "sys_config/"
                                            + fru_map[board]["name"]
                                            + loc_map[sname[8:10]]
                                        )
                                        dimm_sts = kv.kv_get(
                                            board_name, kv.FPERSIST, binary=True
                                        )
                                    if dimm_sts[0] != 1:
                                        return 0
                                    else:
                                        return 1
                                else:
                                    fru_name = c_char_p(board.encode("utf-8"))
                                    snr_name = c_char_p(sname.encode("utf-8"))
                                    is_m2_prsnt = lpal_hndl.pal_is_m2_prsnt(
                                        fru_name, snr_name
                                    )
                                    return int(is_m2_prsnt)
                    else:
                        if match(r"dc_nvme", sname) is not None:  # GPv1
                            fru_name = c_char_p(board.encode("utf-8"))
                            snr_name = c_char_p(sname.encode("utf-8"))
                            is_m2_prsnt = lpal_hndl.pal_is_m2_prsnt(fru_name, snr_name)
                            return int(is_m2_prsnt)
                        return 1
                return 0
        else:
            Logger.debug("Sensor corresponding valid check funciton not found!")
            return 0
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.warn("Exception with board=%s, sensor_name=%s" % (board, sname))
    return 0


def get_fan_mode(scenario="None"):
    if "sensor_hit_UCR" in scenario:
        return fan_mode["boost_mode"], int(90)
    elif "sensor_fail" in scenario:
        return fan_mode["boost_mode"], int(60)
    elif "one_fan_failure" in scenario:
        return fan_mode["trans_mode"], int(95)

    return None, None
