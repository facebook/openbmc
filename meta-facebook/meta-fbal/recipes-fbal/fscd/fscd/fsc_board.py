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
from ctypes import CDLL, c_char_p
from subprocess import PIPE, Popen, check_output

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
    """
    Override the method to define fan specific actions like:
    - handling host power off
    - alarming/syslogging criticals
    """
    Logger.warn("Host needs action %s and cause %s" % (str(action), str(cause)))
    pass


def board_callout(callout="None", **kwargs):
    """
    Override this method for defining board specific callouts:
    - Exmaple chassis intrusion
    """
    if "read_power" in callout:
        return bmc_read_power()
    elif "init_fans" in callout:
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
    with open(
      "/tmp/gpionames/PWRGD_CPU0_LVC3/value", "r"
    ) as f:
      pwr_sts = f.read(1)
    if pwr_sts[0] == "1":
      return 1
    else:
      return 0


def sensor_valid_check(board, sname, check_name, attribute):
    cmd = ""
    data = ""
    try:
        if attribute["type"] == "power_status":
            # check power status first
            pwr_sts = bmc_read_power()
            if pwr_sts != 1:
                return 0

            fru_name = c_char_p(board.encode("utf-8"))
            snr_name = c_char_p(sname.encode("utf-8"))

            is_snr_valid = lpal_hndl.pal_sensor_is_valid(fru_name, snr_name)

            return int(is_snr_valid)

        elif attribute["type"] == "gpio":
            cmd = ["gpiocli", "get-value", "--shadow", attribute["shadow"]]
            data = check_output(cmd).decode().split("=")
            if int(data[1]) == 0:
                return 1
            else:
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
