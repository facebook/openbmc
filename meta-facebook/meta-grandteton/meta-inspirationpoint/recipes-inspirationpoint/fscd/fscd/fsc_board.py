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
import os
import re
from ctypes import c_char_p, CDLL
from subprocess import check_output, PIPE, Popen
from fsc_common_var import fan_mode
from ctypes import byref, c_uint8

import kv
import libgpio
from fsc_util import Logger

lpal_hndl = CDLL("libpal.so.0")

try:
    if lpal_hndl.pal_is_artemis():
        get_fan_mode_scenario_list = {"sensor_hit_UCR": 100}
    else:
        pwr_limit = kv.kv_get("auto_fsc_config", kv.FPERSIST, True).decode("utf-8")
        if pwr_limit == "650":
            get_fan_mode_scenario_list = {"sensor_hit_UCR": 100, "sensor_fail": 100}
        else:
            get_fan_mode_scenario_list = {"sensor_hit_UCR": 100, "sensor_fail": 80}
except Exception:
    # In case of exception, set the default value
    get_fan_mode_scenario_list = {"sensor_hit_UCR": 100, "sensor_fail": 100}
    Logger.warn("FSC can't get fan mode scenario list")

fan_mode = {"normal_mode": 0, "trans_mode": 1, "boost_mode": 2, "progressive_mode": 3}

gta_fru_map = {
    "mb":   {"fru": 1},
    "hpdb": {"fru": 9},
    "vpdb": {"fru": 10},
    "hsc":  {"fru": 14},
    "cb":   {"fru": 16},
    "mc":   {"fru": 17},
}

gta_dimm_map = {
    "a0":   {"dimm_num": 0},
    "a1":   {"dimm_num": 1},
    "a2":   {"dimm_num": 2},
    "a3":   {"dimm_num": 3},
    "a4":   {"dimm_num": 4},
    "a5":   {"dimm_num": 5},
    "a6":   {"dimm_num": 6},
    "a7":   {"dimm_num": 7},
    "a8":   {"dimm_num": 8},
    "a9":   {"dimm_num": 9},
    "a10":  {"dimm_num": 10},
    "a11":  {"dimm_num": 11},
    "b0":   {"dimm_num": 12},
    "b1":   {"dimm_num": 13},
    "b2":   {"dimm_num": 14},
    "b3":   {"dimm_num": 15},
    "b4":   {"dimm_num": 16},
    "b5":   {"dimm_num": 17},
    "b6":   {"dimm_num": 18},
    "b7":   {"dimm_num": 19},
    "b8":   {"dimm_num": 20},
    "b9":   {"dimm_num": 21},
    "b10":  {"dimm_num": 22},
    "b11":  {"dimm_num": 23},
}

gta_asic_fru_map = {
    "accl1":  {"fru": 18},
    "accl2":  {"fru": 19},
    "accl3":  {"fru": 20},
    "accl4":  {"fru": 21},
    "accl5":  {"fru": 22},
    "accl6":  {"fru": 23},
    "accl7":  {"fru": 24},
    "accl8":  {"fru": 25},
    "accl9":  {"fru": 26},
    "accl10": {"fru": 27},
    "accl11": {"fru": 28},
    "accl12": {"fru": 29},
}

def get_fan_mode(scenario="None"):
    if scenario in get_fan_mode_scenario_list:
        return fan_mode["boost_mode"], get_fan_mode_scenario_list[scenario]
    pass

def board_fan_actions(fan, action="None"):
    """
    Override the method to define fan specific actions like:
    - handling dead fan
    - handling fan led
    """
    Logger.warn("%s needs action %s" % (fan.label, str(action)))
    pass


def board_host_actions(action="None", cause="None"):
    FRU_MB = 1
    """
    Override the method to define fan specific actions like:
    - handling host power off
    - alarming/syslogging criticals
    """
    if "host_shutdown" in action:
        Logger.crit(f"FRU: {FRU_MB} FSC Host shutdown action with cause: {cause}")
        lpal_hndl.pal_power_button_override(FRU_MB)
    pass


def board_callout(callout="None", **kwargs):
    """
    Override this method for defining board specific callouts:
    - Exmaple chassis intrusion
    """
    if "read_power" in callout:
        return bmc_read_power()
    elif "init_fans" in callout:
        CM_FRU_ID = 2
        if lpal_hndl.pal_is_fw_update_ongoing(CM_FRU_ID):
            return 0
        boost = 100  # define a boost for the platform or respect fscd override
        if "boost" in kwargs:
            boost = kwargs["boost"]
        Logger.info("FSC init fans to boost=%s " % str(boost))
        return set_all_pwm(boost)
    pass


def set_all_pwm(boost):
    cmd = "/usr/local/bin/fan-util --set %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    response = response.decode()
    return response


def bmc_read_power():
    with libgpio.GPIO(shadow="FM_PWRGD_CPU1_PWROK") as gpio:
        if gpio.get_value() == libgpio.GPIOValue.HIGH:
            return 1
        return 0


def is_dev_prsnt(filename):
    try:
        val = kv.kv_get(filename, kv.FPERSIST, True)
        if val[0] == 1:
            return 1
        return 0

    except Exception:
        return 1


def sensor_valid_check(board, sname, check_name, attribute):
    cmd = ""
    data = ""
    if str(board) == "all":
        sdata = sname.split("_")
        board = sdata[0]
        sname = sname.replace(board + "_", "")

    try:
        if attribute["type"] == "power_status":
            # TODO: Check nvme ready/present/specific access check for Dev
            if (lpal_hndl.pal_is_artemis() == True):
                if (lpal_hndl.pal_is_fw_update_ongoing(int(gta_fru_map[board]["fru"]))== True):
                    return 0
                if re.match(r"(.*)dimm(.*)", sname) is not None:
                    snr_split = sname.split("_")
                    if (lpal_hndl.is_dimm_present(int(gta_dimm_map[snr_split[2]]["dimm_num"])) == False):
                        return 0
                if re.match(r"(.*)accl(.*)", sname) is not None:
                    accl_present = c_uint8(0)
                    snr_split = sname.split("_")
                    ret = lpal_hndl.pal_is_fru_prsnt(int(gta_asic_fru_map[snr_split[0]]["fru"]), byref(accl_present))
                    if ret == 0:
                        # Not Present
                        if (accl_present.value == 0):
                            return 0
                    else:
                        return 0
            # check power status first
            pwr_sts = bmc_read_power()
            if pwr_sts == 1:
                if (lpal_hndl.pal_is_artemis() == True):
                    if lpal_hndl.pal_bios_completed(1) == False:
                        return 0
                # if re.match(r"(.*)dimm(.*)", sname) is not None:
                #     snr_split = sname.split("_")
                #     cpu_num = int(snr_split[4][3])
                #     pos = int(snr_split[6][1])
                #     dimm_num = str(cpu_num * 12 + pos * 2)
                #     dimm_name = "sys_config/fru1_dimm" + dimm_num + "_location"
                #     return is_dev_prsnt(dimm_name)
                return 1
            return 0
        else:
            Logger.debug("Sensor corresponding valid check funciton not found!")
            return -1
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception as err:
        Logger.crit(
            "Exception with board=%s, sensor_name=%s, cmd=%s, response=%s, err=%s"
            % (board, sname, cmd, data, err)
        )
    return 0
