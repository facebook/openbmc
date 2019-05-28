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
from fsc_control import PID, TTable
from fsc_sensor import FscSensorSourceSysfs, FscSensorSourceUtil
from fsc_util import Logger


class Sensor(object):
    def __init__(self, sensor_name, pTable):
        try:
            if "read_source" in pTable:
                if "sysfs" in pTable["read_source"]:
                    self.source = FscSensorSourceSysfs(
                        name=sensor_name, read_source=pTable["read_source"]["sysfs"]
                    )
                if "util" in pTable["read_source"]:
                    self.source = FscSensorSourceUtil(
                        name=sensor_name, read_source=pTable["read_source"]["util"]
                    )
        except Exception:
            Logger.error("Unknown Sensor source type")


def profile_constructor(data):
    return lambda: make_controller(data)


def make_controller(pTable):
    if pTable["type"] == "linear":
        controller = TTable(
            pTable["data"],
            pTable.get("negative_hysteresis", 0),
            pTable.get("positive_hysteresis", 0),
        )
        return controller
    if pTable["type"] == "pid":
        controller = PID(
            pTable["setpoint"],
            pTable["kp"],
            pTable["ki"],
            pTable["kd"],
            pTable["negative_hysteresis"],
            pTable["positive_hysteresis"],
        )
        return controller
    err = "Don't understand profile type '%s'" % (pTable["type"])
    Logger.error(err)
