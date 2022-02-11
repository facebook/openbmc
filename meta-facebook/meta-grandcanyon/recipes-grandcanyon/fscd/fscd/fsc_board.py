#!/usr/bin/env python
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
from kv import kv_get

lpal_hndl = CDLL("libpal.so.0")

SDR_BIN_PATH = "/tmp/sdr_server.bin"

def board_fan_actions(fan, action="None"):
    if action in "dead":
        # TODO: Need to implement function to handle fan dead.
        pal_fan_dead_handle(fan.fan_num + 1)
    elif action in "recover":
        # TODO: Need to implement function to handle fan recovered.
        pal_fan_recovered_handle(fan.fan_num + 1)
    elif "led" in action:
        pass
    else:
        Logger.warn("%s needs action %s" % (fan.label, str(action)))
    pass

def board_host_actions(action="None", cause="None"):
    """
    Override the method to define fan specific actions like:
    - handling host power off
    - alarming/syslogging critical
    """
    pass

def board_callout(callout="None", **kwargs):
    if "init_fans" in callout:
        boost = 80
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

def sensor_valid_check(board, sname, check_name, attribute):
    cmd = ""
    data = ""
    is_present = 0
    is_ready = 1
    is_enable = 1
    try:
        if attribute["type"] == "host_power_status":
            cmd = "/usr/local/bin/power-util %s status" % attribute["fru"]
            data = ""
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            result = data.split(": ")
            if match(r"ON", result[1]) is not None:
                cmd = "/usr/local/bin/gpiocli -c aspeed-gpio -s %s get-value" % attribute["bic_ready_pin"]
                data = ""
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
                bic_ready = data.split("=")
                if int(bic_ready[1]) == is_ready:
                    if os.path.isfile(SDR_BIN_PATH):
                        return 1
                    else:
                        return 0
                else:
                    return 0
            else:
                return 0
        elif attribute["type"] == "E1S_status":
            cmd = "/usr/local/bin/gpiocli -c aspeed-gpio -s %s get-value" % attribute["gpio_present_pin"]
            data = ""
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            e1s_present = data.split("=")

            cmd = "/usr/local/bin/gpiocli -c aspeed-gpio -s %s get-value" % attribute["e1s_i2c_enable"]
            data = ""
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            e1s_i2c_enable = data.split("=")

            if int(e1s_present[1]) == is_present and int(e1s_i2c_enable[1]) == is_enable:
                return 1
            else:
                return 0
        else:
            Logger.debug("Sensor corresponding valid check function not found!")
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
