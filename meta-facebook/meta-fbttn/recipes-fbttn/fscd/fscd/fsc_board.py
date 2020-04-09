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
import os.path
from ctypes import CDLL, c_uint, pointer
from re import match
from subprocess import PIPE, Popen

from fsc_util import Logger


lpal_hndl = CDLL("libpal.so.0")


def board_fan_actions(fan, action="None"):
    """
    Override the method to define fan specific actions like:
    - handling dead fan
    - handling fan led
    """
    if action in "dead":
        # TODO: Why not do return pal_fan_dead_handle(fan)
        pal_fan_dead_handle(fan.fan_num + 1)
    elif action in "recover":
        # TODO: Why not do return pal_fan_recovered_handle(fan)
        pal_fan_recovered_handle(fan.fan_num + 1)
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
    if "chassis_intrusion" in callout:
        return pal_fan_chassis_intrusion_handle()
    elif "init_fans" in callout:
        boost = 100  # define a boost for the platform or respect fscd override
        if "boost" in kwargs:
            boost = kwargs["boost"]
        return set_all_pwm(boost)
    else:
        Logger.warn("Callout %s not handled" % callout)


def set_all_pwm(boost):
    cmd = "/usr/local/bin/fan-util --set %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    return response


def pal_fan_dead_handle(fan):
    ret = lpal_hndl.pal_fan_dead_handle(fan)
    if ret:
        return None
    else:
        return ret


def pal_fan_recovered_handle(fan):
    ret = lpal_hndl.pal_fan_recovered_handle(fan)
    if ret:
        return None
    else:
        return ret


def pal_fan_chassis_intrusion_handle():
    self_tray_pull_out = c_uint(1)
    self_tray_pull_out_point = pointer(self_tray_pull_out)
    ret = lpal_hndl.pal_self_tray_location(self_tray_pull_out_point)
    if ret:
        return None
    else:
        return self_tray_pull_out.value


def sensor_valid_check(board, sname, check_name, attribute):
    cmd = ""
    data = ""
    try:
        if attribute["type"] == "power_BIC_status":
            cmd = "/usr/local/bin/power-util %s status" % attribute["fru"]
            data = ""
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            result = data.split(": ")
            if match(r"ON", result[1]) is not None:
                cmd = "cat /sys/class/gpio/gpio%s/value" % attribute["number"]
                data = ""
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
                if int(data) == 0:
                    return 1
                else:
                    return 0
            else:
                return 0

        elif attribute["type"] == "gpio_power_nvme":
            cmd = "cat /sys/class/gpio/gpio%s/value" % attribute["number"]
            data = ""
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            if int(data) == 0:
                cmd = "/usr/local/bin/power-util %s status" % attribute["fru"]
                data = ""
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
                result = data.split(": ")
                if match(r"ON", result[1]) is not None:
                    cmd = "/tmp/cache_store/M2_%s_NVMe" % attribute["nvme"]
                    data = ""
                    if os.path.isfile(cmd) is True:
                        with open(cmd, "r") as data:
                            if data.read() == "1":
                                return 1
                            else:
                                return 0
                    else:
                        return 0
                else:
                    return 0
            else:
                return 0
        else:
            Logger.debug("Sensor corresponding valid check funciton not found!")
            return 0
    except SystemExit:
        Logger.debug("SystemExit from sensor read")
        raise
    except Exception:
        Logger.crit(
            "Exception with board=%s, sensor_name=%s, cmd=%s, response=%s"
            % (board, sname, cmd, data)
        )
    return 0
