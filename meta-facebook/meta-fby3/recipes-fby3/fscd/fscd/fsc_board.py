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
import os
import re
import struct
import time
from ctypes import CDLL, c_uint8, byref
from re import search
from subprocess import PIPE, Popen

import kv
from fsc_util import Logger
from kv import FPERSIST, kv_get

fan_mode = {
    "normal_mode": 0,
    "trans_mode": 1,
    "boost_mode": 2,
    "progressive_mode": 3}
get_fan_mode_scenario_list = ["one_fan_failure", "sensor_hit_UCR"]
sled_system_conf = "Type_NA"
try:
    sled_system_conf = kv_get("sled_system_conf", FPERSIST)
    Logger.warn("sled_system_conf: %s" % (sled_system_conf))

    # Add sensor fail scenario for DP case
    if search(r"Type_DP", sled_system_conf) is not None:
        Logger.warn("Add sensor fail scenario for DP case")
        get_fan_mode_scenario_list = [
            "one_fan_failure",
            "sensor_hit_UCR",
            "sensor_fail",
        ]
except kv.KeyNotFoundFailure:
    Logger.warn("KeyNotFoundFailure from sled_system_conf")

lpal_hndl = CDLL("libpal.so.0")

fru_map = {
    "slot1": {"name": "fru1", "slot_num": 1},
    "slot2": {"name": "fru2", "slot_num": 2},
    "slot3": {"name": "fru3", "slot_num": 3},
    "slot4": {"name": "fru4", "slot_num": 4},
    "slot1_2U_exp": {"name": "fru1", "slot_num": 1},
    "slot1_2U_top": {"name": "fru1", "slot_num": 1},
    "slot1_2U_bot": {"name": "fru1", "slot_num": 1},
}

cwc_fru_map = {
    "slot1_2U_exp": {"fru": 31},
    "slot1_2U_top": {"fru": 32},
    "slot1_2U_bot": {"fru": 33},
}

# PESW_FW id which is defined in bic.h
pesw_target_map = {
    "slot1_2U_exp": {"target_id": 47},
    "slot1_2U_top": {"target_id": 51},
    "slot1_2U_bot": {"target_id": 55},
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
    "slot1_2U_exp": "fru1_host_ready",
    "slot1_2U_top": "fru1_host_ready",
    "slot1_2U_bot": "fru1_host_ready",
}

# boot drive sensor number which is defined in pal_sensors.h
boot_drive_map = {
    "slot1": {"sensor_num": 0xE},
}

valid_dp_pcie_list = [
    # Vendor Device
    (0x8086, 0x10D8)  # Marvell HSM
]


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


def is_valid_dp_pcie(pcie_info_key):
    try:
        pcie_info_raw = kv_get(pcie_info_key, flags=kv.FPERSIST, binary=True)
        pcie_info_data = struct.unpack_from("<HH", pcie_info_raw, 2)
        if pcie_info_data in valid_dp_pcie_list:
            return 1
        else:
            return 0
    except kv.KeyOperationFailure:
        Logger.warn("Exception KeyOperationFailure, key=%s" % (pcie_info_key))
        # return not valid if pcie_info file is not ready
        return 0


def is_dev_prsnt(filename):
    dev_prsnt = 0
    if not os.path.exists(filename):
        return False

    # read byte1
    with open(filename, "rb") as f:
        dev_prsnt = f.read(1)

    if dev_prsnt == b"\x01":
        return True

    return False


def sensor_valid_check(board, sname, check_name, attribute):
    if str(board) == "all":
        sdata = sname.split("_")
        board = sdata[0]
        sname = sname.replace(board + "_", "")

    status = c_uint8(0)
    is_valid_check = False
    try:
        file = "/var/run/power-util_%d.lock" % int(fru_map[board]["slot_num"])
        if os.path.exists(file):
            return 0

        if attribute["type"] == "power_status":
            lpal_hndl.pal_get_server_power(
                int(fru_map[board]["slot_num"]), byref(status)
            )
            if status.value == 6:  # 12V-off
                return 0

            if (search(r"front_io_temp", sname) is not None) and (
                status.value == 0 or status.value == 1
            ):
                return 1

            # If sensor fail, dp will boost without checking host ready
            if (search(r"Type_DP", sled_system_conf) is None) or (
                search(r"Type_DPB", sled_system_conf) is not None
            ):
                try:
                    flag_status = kv_get(host_ready_map[board])
                except kv.KeyOperationFailure:
                    return 0

                if flag_status != "1":
                    return 0

            if status.value == 1:  # power on
                if search(r"soc_cpu|soc_therm", sname) is not None:
                    is_valid_check = True
                elif search(r"spe_ssd", sname) is not None:
                    # get SSD present status
                    cmd = "/usr/bin/bic-util slot1 0xe0 0x2 0x9c 0x9c 0x0 0x15 0xe0 0x34 0x9c 0x9c 0x0 0x0 0x3"
                    response = Popen(
                        cmd, shell=True, stdout=PIPE).stdout.read()
                    response = response.decode()
                    # check the completion code
                    if response.split(" ")[6] != "00":
                        return 0
                    prsnt_bits = response.split(" ")[-3]
                    int_val = int("0x" + prsnt_bits, 16)
                    ssd_id = int(sname[7])
                    if int_val & (1 << ssd_id):
                        return 1
                    else:
                        return 0
                elif search(r"dp_marvell_hsm_t", sname) is not None:
                    pcie_info_key = (
                        "sys_config/" +
                        fru_map[board]["name"] +
                        "_pcie_i04_s40_info")
                    return is_valid_dp_pcie(pcie_info_key)
                elif search(r"gp3_m2", sname) is not None:
                    # get GPv3 M.2 device status
                    nvme_ready = c_uint8(0)
                    dev_status = c_uint8(0)
                    dev_type = c_uint8(0)
                    if board == "slot1_2U_top" or board == "slot1_2U_bot":
                        if sname[8] != "_":
                            # gp3_m2_10_temp
                            dev_id = int(sname[7]) * 10 + int(sname[8]) + 1
                        else:
                            dev_id = int(sname[7]) + 1  # gp3_m2_0_temp
                        response = lpal_hndl.pal_get_dev_info(
                            int(cwc_fru_map[board]["fru"]),
                            dev_id,
                            byref(nvme_ready),
                            byref(dev_status),
                            byref(dev_type),
                        )
                        if response == 0:  # bic can get device power status
                            if dev_status.value == 1 and nvme_ready.value == 1:  # nvme is ready
                                return 1
                            else:
                                return 0
                        else:
                            return 0
                    else:
                        dev_id = int(sdata[3]) + 1
                        response = lpal_hndl.pal_get_dev_info(
                            int(fru_map[board]["slot_num"]),
                            dev_id,
                            byref(nvme_ready),
                            byref(dev_status),
                            byref(dev_type),
                        )
                        if response == 0:  # bic can get device power status
                            if dev_status.value == 1:  # device power on
                                return 1
                            else:
                                return 0
                        else:
                            return 0
                elif search(r"gp3_e1s", sname) is not None:
                    # get GPv3 E1.S device status
                    nvme_ready = c_uint8(0)
                    dev_status = c_uint8(0)
                    dev_type = c_uint8(0)
                    dev_id = int(sname[7]) + 13
                    if board == "slot1_2U_top" or board == "slot1_2U_bot":
                        response = lpal_hndl.pal_get_dev_info(
                            int(cwc_fru_map[board]["fru"]),
                            dev_id,
                            byref(nvme_ready),
                            byref(dev_status),
                            byref(dev_type),
                        )
                    else:
                        response = lpal_hndl.pal_get_dev_info(
                            int(fru_map[board]["slot_num"]),
                            dev_id,
                            byref(nvme_ready),
                            byref(dev_status),
                            byref(dev_type),
                        )
                    if response == 0:  # bic can get device power status
                        if dev_status.value == 1:  # device power on
                            return 1
                        else:
                            return 0
                    else:
                        return 0
                elif search(r"(.*)pesw_temp", sname) is not None:  # get CWC or GPv3 PESW fw info
                    pesw_res = c_uint8(0)
                    pesw_res_len = c_uint8(0)
                    response = lpal_hndl.pal_get_fw_info(
                        int(fru_map[board]["slot_num"]),
                        int(pesw_target_map[board]["target_id"]),
                        byref(pesw_res),
                        byref(pesw_res_len),
                    )
                    if response == -1:  # can not get fw info
                        return 0
                    else:
                        return 1
                # check if boot driver supports sensor reading
                elif search(r"ssd", sname) is not None:
                    response = lpal_hndl.pal_is_sensor_valid(
                        int(fru_map[board]["slot_num"]),
                        boot_drive_map[board]["sensor_num"],
                    )
                    if response == 1:  # boot drive unsupports sensor reading
                        return 0
                    else:
                        return 1
                else:
                    suffix = ""
                    if search(r"1ou_m2", sname) is not None:
                        # 1ou_m2_a_temp. key is at 7
                        suffix = m2_1ou_name_map[sname[7]]
                    elif search(r"soc_dimm", sname) is not None:
                        # soc_dimmf_temp. key is at 8
                        suffix = dimm_location_name_map[sname[8]]

                    file = (
                        "/mnt/data/kv_store/sys_config/"
                        + fru_map[board]["name"]
                        + suffix
                    )
                    if is_dev_prsnt(file) == True:
                        is_valid_check = True

            if is_valid_check == True:
                # Check power status again
                lpal_hndl.pal_get_server_power(
                    int(fru_map[board]["slot_num"]), byref(status)
                )
                if status.value == 1:  # power on
                    if (search(r"Type_DP", sled_system_conf) is None) or (
                        search(r"Type_DPB", sled_system_conf) is not None
                    ):
                        try:
                            flag_status = kv_get(host_ready_map[board])
                        except kv.KeyOperationFailure:
                            return 0

                        if flag_status == "1":
                            return 1
                    else:
                        # Type DP or DPB
                        return 1
                else:
                    return 0

        return 0
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.warn("Exception with board=%s, sensor_name=%s" % (board, sname))
    return 0


def get_fan_mode(scenario="None"):
    if "one_fan_failure" in scenario:
        sled_system_conf = "Type_NA"
        try:
            sled_system_conf = kv_get("sled_system_conf", FPERSIST)
        except kv.KeyNotFoundFailure:
            Logger.warn("KeyNotFoundFailure from sled_system_conf")
        if lpal_hndl.pal_is_cwc() == 0:
            pwm = 80
        elif sled_system_conf == "Type_15":
            # config D GPv3
            pwm = 80
            return fan_mode["boost_mode"], pwm
        else:
            pwm = 60
        return fan_mode["trans_mode"], pwm
    elif "sensor_hit_UCR" in scenario:
        pwm = 100
        return fan_mode["boost_mode"], pwm
    elif "sensor_fail" in scenario:
        pwm = 100
        return fan_mode["boost_mode"], pwm
    pass


def fru_format_transform(fru):
    if fru == "slot1_2U_top":
        return "slot1-2U-top"
    elif fru == "slot1_2U_bot":
        return "slot1-2U-bot"
    elif fru == "slot1_2U_exp":
        return "slot1-2U-exp"
    else:
        return fru
