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
import time
from subprocess import PIPE, Popen

from fsc_util import Logger


# Overrides the functions that need to do HW dependent
# actions, such as LED control, PWM full boost and
# emergency shutdown


def elbert_set_fan_led(fan, color="led_blue"):
    FCM_CPLD = "/sys/bus/i2c/drivers/fancpld/6-0060/"
    for fan in fan.split():
        # ELBERT fans are all named as number
        if not fan.isdigit():
            break
        fan = int(fan)
        if fan > 5 or fan < 1:
            break

        # 1 means off
        green_value = 1
        red_value = 1
        blue_value = 1
        amber_value = 1
        green_key = FCM_CPLD + "fan" + str(fan) + "_led_green"
        red_key = FCM_CPLD + "fan" + str(fan) + "_led_red"
        blue_key = FCM_CPLD + "fan" + str(fan) + "_led_blue"
        amber_key = FCM_CPLD + "fan" + str(fan) + "_led_amber"

        # 0 means on
        if "red" in color:
            red_value = 0
        elif "blue" in color:
            blue_value = 0
        elif "green" in color:
            green_value = 0
        elif "amber" in color:
            amber_value = 0
        else:
            return 0  # error

        cmd_green = "echo " + str(green_value) + " > " + green_key
        cmd_red = "echo " + str(red_value) + " > " + red_key
        cmd_blue = "echo " + str(blue_value) + " > " + blue_key
        cmd_amber = "echo " + str(amber_value) + " > " + amber_key

        # Do the best effort. When LED setting commands fail, move on,
        # as this failure alone will not damage or take down hardware
        Popen(cmd_green, shell=True, stdout=PIPE).stdout.read()
        Popen(cmd_red, shell=True, stdout=PIPE).stdout.read()
        Popen(cmd_blue, shell=True, stdout=PIPE).stdout.read()
        Popen(cmd_amber, shell=True, stdout=PIPE).stdout.read()
    return 0


def board_fan_actions(fan, action="None"):
    if "led" in action:
        elbert_set_fan_led(fan.label, color=action)
    else:
        Logger.warn("fscd: %s has no action %s" % (fan.label, str(action)))
    pass


def elbert_set_all_pwm(boost):
    # The script name is same as Minipack.
    # Note that, the argument to this script is in range [0,100]
    # and the script will scale the value to [0,255]
    cmd = "/usr/local/bin/set_fan_speed.sh %d" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    return response


def board_callout(callout="None", **kwargs):
    if "init_fans" in callout:
        boost = 100
        if "boost" in kwargs:
            boost = kwargs["boost"]
        Logger.info("FSC init fans to boost=%s " % str(boost))
        return elbert_set_all_pwm(boost)
    else:
        Logger.warn("Need to perform callout action %s" % callout)
    pass


# This function will run cmd_str and ignore any exception
# We use this function for shutdown sequence, as the system may
# be in a bad state where any command may fail
def elbert_force_run_cmd(cmd_str):
    try:
        Popen(cmd_str, shell=True, stdout=PIPE).stdout.read()
    except Exception:
        pass
    return 0


def elbert_host_shutdown():
    # Do the best effort by :
    # 1. Turn off CPU
    # 2. Then turn off SMB
    # 3. Then turn off all PSU
    # We do this because, if any one of CPLD/FPGA is already
    # malfunctioning, we still want to turn off as much part of
    # the system as possible.
    SMB_POWER_REG = "/sys/bus/i2c/drivers/smbcpld/4-0023/smb_power_en"
    CPU_OFF = "/usr/local/bin/wedge_power.sh off"
    SMB_OFF = "echo 0 > " + SMB_POWER_REG
    # First, turn off most of the switch board
    Logger.info("host_shutdown() executing {}".format(SMB_OFF))
    elbert_force_run_cmd(SMB_OFF)
    time.sleep(3)
    # Then, turn off X86 CPU
    Logger.info("host_shutdown() executing {}".format(CPU_OFF))
    elbert_force_run_cmd(CPU_OFF)

    # Until FSCD is proven to be very stable on most versions of FSCD,
    # we will only turn off SMB and BMC, but not PSUs.
    # (When PSUs are all turned off, it's hard to recover in DC)

    return 0


def board_host_actions(action="None", cause="None"):
    if "host_shutdown" in action:
        Logger.crit("Host is shutdown due to cause %s" % (str(cause),))
        return elbert_host_shutdown()
    Logger.warn("Host needs action '%s' and cause '%s'" % (str(action), str(cause)))
    pass
