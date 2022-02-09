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

        if fan_presence < 6:
            Logger.warn("chassis_intrusion Found Fan absent (%d/6)" % (fan_presence))
            tray_pull_out = 1
        if psu_presence < 2:
            Logger.warn("chassis_intrusion Found PSU absent (%d/2)" % (psu_presence))
        return tray_pull_out
    else:
        Logger.warn("Need to perform callout action %s" % callout)
    pass


def set_all_pwm(boost):
    # script will take care of conversion
    cmd = "/usr/local/bin/set_fan_speed.sh %d all both" % (boost)
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    return response


def set_fan_led(fan, color="led_blue"): 
    # Agc032a led color is green.
    # In order to fit common layer code, remain default color blue
    FAN_LED_GREEN = "0x1"
    FAN_LED_RED = "0x2"

    for fan in fan.split():
        if fan.isdigit():
            break

    fan = int(fan)
    FAN_LED = "/sys/bus/i2c/drivers/swpld_1/7-0032/"

    fan_key = "fan%d_led" % fan
    if "red" in color:
        cmd = "echo " + FAN_LED_RED + " > " + FAN_LED + fan_key
    elif "blue" in color:
        cmd = "echo " + FAN_LED_GREEN + " > " + FAN_LED + fan_key
    else:
        return 0  # error
    # Logger.debug("Using cmd=%s for led" % str(cmd))
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    return 0


def host_shutdown():
    MAIN_POWER = "/sys/bus/i2c/drivers/smb_syscpld/12-003e/cpld_in_p1220"
    USERVER_POWER = "/sys/bus/i2c/drivers/scmcpld/2-003e/com_exp_pwr_enable"

    cmd = "echo 0 > " + USERVER_POWER
    Logger.info("host_shutdown() executing {}".format(cmd))
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    time.sleep(5)
    cmd = "echo 0 > " + MAIN_POWER
    Logger.info("host_shutdown() executing {}".format(cmd))
    response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    return response
