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

import kv
import libgpio
from fsc_util import Logger

lpal_hndl = CDLL("libpal.so.0")


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

    try:
        if attribute["type"] == "power_status":
            # check power status first
            pwr_sts = bmc_read_power()
            if pwr_sts == 1:
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
