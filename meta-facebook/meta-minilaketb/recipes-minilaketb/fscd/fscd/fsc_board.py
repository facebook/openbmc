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
from ctypes import CDLL
from re import match
from subprocess import PIPE, Popen

from fsc_util import Logger


lpal_hndl = CDLL("libpal.so.0")

fru_map = {"slot1": {"name": "fru1", "gpio": "64"}}

loc_map = {"a0": "_dimm0_location", "b0": "_dimm1_location"}


def board_fan_actions(fan, action="None"):
    """
    Override the method to define fan specific actions like:
    - handling dead fan
    - handling fan led
    """
    if action in "dead":
        # TODO: Why not do return pal_fan_dead_handle(fan)
        lpal_hndl.pal_fan_dead_handle(fan.fan_num + 1)
    elif action in "recover":
        # TODO: Why not do return pal_fan_recovered_handle(fan)
        lpal_hndl.pal_fan_recovered_handle(fan.fan_num + 1)
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
        Logger.warning("Callout %s not handled" % callout)
    pass


def set_all_pwm(boost):
    cmd = "/usr/local/bin/fan-util --set %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    response = response.decode()
    return response


def sensor_valid_check(board, sname, check_name, attribute):
    try:
        if attribute["type"] == "power_status":
            with open(
                "/sys/class/gpio/gpio" + fru_map[board]["gpio"] + "/value", "r"
            ) as f:
                pwr_sts = f.read(1)
            if pwr_sts[0] == "1":
                if match(r"soc_dimm", sname) != None:
                    # check DIMM present
                    with open(
                        "/mnt/data/kv_store/sys_config/"
                        + fru_map[board]["name"]
                        + loc_map[sname[8:10]],
                        "rb",
                    ) as f:
                        dimm_sts = f.read(1)
                    if dimm_sts[0] != 1:
                        return 0
                return 1
            else:
                return 0
        else:
            Logger.debug("Sensor corresponding valid check funciton not found!")
            return -1
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.warn("Exception with board=%s, sensor_name=%s" % (board, sname))
    return -1
