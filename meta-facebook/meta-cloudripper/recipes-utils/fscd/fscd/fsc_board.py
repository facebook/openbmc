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
import time
from subprocess import PIPE, Popen

from fsc_util import Logger
from ctypes import CDLL, byref, c_uint8, create_string_buffer


lpal_hndl = CDLL("libpal.so.0")


def board_fan_actions(fan, action="None"):
    """
    Override the method to define fan specific actions like:
    - handling dead fan
    - handling fan led
    """
    if "led" in action:
        set_fan_led(fan.label, color=action)
    else:
        Logger.info("%s needs action %s" % (fan.label, str(action)))
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
        fru_list = ["fan1", "fan2", "fan3", "fan4", "psu1", "psu2"]
        fru_id = c_uint8()
        state = c_uint8()
        tray_pull_out = 0
        for fru_name in fru_list:
            if not hasattr(board_callout, fru_name):
                setattr(board_callout, fru_name, "none")
            last_state = getattr(board_callout, fru_name)

            fru_name_p = create_string_buffer(fru_name.encode(), 8)
            if lpal_hndl.pal_get_fru_id(byref(fru_name_p), byref(fru_id)):
                Logger.error("chassis_intrusion can't get fru id of %s" % fru_name)
                continue
            if lpal_hndl.pal_is_fru_prsnt(fru_id.value, byref(state)):
                Logger.error("chassis_intrusion can't get fru status of %s" % fru_name)
                continue

            if (
                "fan" in fru_name and state.value == 0
            ):  # when fan module absent, set fan speed at 70%
                tray_pull_out = 1
            if state.value != last_state:
                if state.value == 0:
                    Logger.warn(
                        "chassis_intrusion %s has been absent" % fru_name.upper()
                    )
                elif state.value == 1:
                    Logger.warn(
                        "chassis_intrusion %s has been present" % fru_name.upper()
                    )
                setattr(board_callout, fru_name, state.value)
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
    FAN_LED = "/sys/bus/i2c/drivers/fcbcpld/48-003e/"

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
    SCM_POWER_COMMAND = (
        "/usr/local/bin/wdtcli kick &> /dev/null; /usr/local/bin/wedge_power.sh off"
    )
    switch_poweroff_cmd = (
        "source /usr/local/bin/openbmc-utils.sh; echo 0 > $SMBCPLD_SYSFS_DIR/gb_turn_on"
    )

    Logger.info("host_shutdown() executing {}".format(SCM_POWER_COMMAND))
    response = Popen(SCM_POWER_COMMAND, shell=True, stdout=PIPE).stdout.read()
    time.sleep(5)
    if switch_poweroff_cmd != "":
        Logger.info("host_shutdown() executing {}".format(switch_poweroff_cmd))
        response = Popen(switch_poweroff_cmd, shell=True, stdout=PIPE).stdout.read()

    return response
