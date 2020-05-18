# Copyright 2019-present Facebook. All Rights Reserved.
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
import time
import re
from subprocess import PIPE, Popen

from fsc_util import Logger
from ctypes import CDLL, byref, c_uint8


lpal_hndl = CDLL("libpal.so.0")


def pal_get_board_type():
    """ get board type """
    BRD_TYPE_WEDGE400 = 0x00
    BRD_TYPE_WEDGE400C = 0x01

    board_type = {
        BRD_TYPE_WEDGE400: "Wedge400",
        BRD_TYPE_WEDGE400C: "Wedge400C",
    }
    brd_type = c_uint8()
    ret = lpal_hndl.pal_get_board_type(byref(brd_type))
    if ret:
        return None
    else:
        return board_type.get(brd_type.value, None)


def board_fan_actions(fan, action="None"):
    """
    Override the method to define fan specific actions like:
    - handling dead fan
    - handling fan led
    """
    if "led" in action:
        set_fan_led(fan.label, color=action)
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
        Logger.crit("Host is shutdown due to cause %s" % (str(cause),))
        return host_shutdown()
    Logger.warn("Host needs action '%s' and cause '%s'" % (str(action), str(cause)))
    pass


def board_callout(callout="None", **kwargs):
    """
    Override this method for defining board specific callouts:
    - Exmaple chassis intrusion
    """
    if "init_fans" in callout:
        boost = 100
        if "boost" in kwargs:
            boost = kwargs["boost"]
        Logger.info("FSC init fans to boost=%s " % str(boost))
        return set_all_pwm(boost)
    elif "chassis_intrusion" in callout:
        # fan present
        cmd = "presence_util.sh fan"
        lines = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        fan_presence = 0
        psu_presence = 0
        tray_pull_out = 0
        for line in lines.split("\n"):
            m = re.match(r"fan.*\s:\s+(\d+)", line)
            if m is not None:
                if int(m.group(1)) == 1:
                    fan_presence += 1

        # psu present
        cmd = "presence_util.sh psu"
        lines = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        for line in lines.split("\n"):
            m = re.match(r"psu.*\s:\s+(\d+)", line)
            if m is not None:
                if int(m.group(1)) == 1:
                    psu_presence += 1

        if fan_presence < 4:
            Logger.warn("chassis_intrusion Found Fan absent (%d/4)" % (fan_presence))
            tray_pull_out = 1
        if psu_presence < 2:
            Logger.warn("chassis_intrusion Found PSU absent (%d/2)" % (psu_presence))
        return tray_pull_out
    else:
        Logger.warn("Need to perform callout action %s" % callout)
    pass


def set_all_pwm(boost):
    # script will take care of conversion
    cmd = "/usr/local/bin/set_fan_speed.sh %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    return response


def set_fan_led(fan, color="led_blue"):
    FAN_LED_BLUE = "0x1"
    FAN_LED_RED = "0x2"

    for fan in fan.split():
        if fan.isdigit():
            break

    fan = int(fan)
    FAN_LED = "/sys/bus/i2c/drivers/fcbcpld/30-003e/"

    fan_key = "fan%d_led_ctrl" % fan
    if "red" in color:
        cmd = "echo " + FAN_LED_RED + " > " + FAN_LED + fan_key
    elif "blue" in color:
        cmd = "echo " + FAN_LED_BLUE + " > " + FAN_LED + fan_key
    else:
        return 0  # error
    # Logger.debug("Using cmd=%s for led" % str(cmd))
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    return 0


def host_shutdown():
    SCM_POWER_COMMAND = "/usr/local/bin/wedge_power.sh reset"
    TH_SWITCH_POWER_COMMAND = "/usr/local/bin/reset_brcm.sh"
    GB_SWITCH_POWER_COMMAND = "/usr/local/bin/switch_reset.sh cycle"
    switch_reset_cmd = ""
    brd_type = pal_get_board_type()
    if brd_type == "Wedge400":
        switch_reset_cmd = TH_SWITCH_POWER_COMMAND
    elif brd_type == "Wedge400C":
        switch_reset_cmd = GB_SWITCH_POWER_COMMAND
    else:
        Logger.crit("Cannot identify board type: %s" % brd_type)
        Logger.crit("Switch won't be resetting!")

    Logger.info("host_shutdown() executing {}".format(SCM_POWER_COMMAND))
    response = Popen(SCM_POWER_COMMAND, shell=True, stdout=PIPE).stdout.read()
    time.sleep(5)
    if switch_reset_cmd != "":
        Logger.info("host_shutdown() executing {}".format(switch_reset_cmd))
        response = Popen(switch_reset_cmd, shell=True, stdout=PIPE).stdout.read()
    return response
