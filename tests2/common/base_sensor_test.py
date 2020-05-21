#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
import re
import json
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseSensorsTest(object):
    def setUp(self):
        """
        For now the scope if for sensors exposed from lm sensors
        """
        Logger.start(name=self._testMethodName)
        self.sensors_cmd = None
        self.parsed_result = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_sensors_cmd(self):
        pass

    @abstractmethod
    def parse_sensors(self):
        pass


class LmSensorsTest(BaseSensorsTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors"]

    def get_parsed_result(self):
        if not self.parsed_result:
            self.parsed_result = self.parse_sensors()
        return self.parsed_result

    def parse_sensors(self):
        self.set_sensors_cmd()
        data = run_shell_cmd(self.sensors_cmd)
        result = {}

        data = re.sub(r"\(.+?\)", "", data)
        for edata in data.split("\n\n"):
            adata = edata.split("\n", 1)
            if len(adata) < 2:
                break
            for sdata in adata[1].split("\n"):
                tdata = sdata.split(":")
                if len(tdata) < 2:
                    continue
                if "Adapter" in tdata:
                    continue
                result[tdata[0].strip()] = tdata[1].strip()
        return result


class SensorUtilTest(BaseSensorsTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensor-util all"]

    def get_parsed_result(self):
        if not self.parsed_result:
            self.parsed_result = self.parse_sensors()
        return self.parsed_result

    def parse_sensors(self):
        self.set_sensors_cmd()
        data = run_shell_cmd(self.sensors_cmd)
        if "not present" in data:
            return {"present": False}
        result = {"present": True}
        # VCCGBE VR Vol                (0x54) :    1.05 Volts | (ok)
        # SOC DIMMA1 Temp              (0xB5) : NA | (na)
        name_regex = re.compile(r"^(.+)\s+\(0x.+\)\s*")

        for edata in data.split("\n"):
            try:
                adata = edata.split(" : ")
                sensor_name = name_regex.match(adata[0]).group(1).strip()
                value_group = adata[1].split(" | ")
                value_units = value_group[0].strip().split(" ")
                sensor_value = value_units[0].strip()
                if sensor_value == "NA":
                    sensor_value = str(0)
                result[sensor_name] = sensor_value
                # Save for future use.
                # sensor_status = value_group[1]
                # sensor_units = "NA"
                # if len(value_units) == 2:
                #     sensor_units = value_units[1].strip()
            except Exception:
                Logger.error("Cannot parse: {}".format(edata))
        return result

    def get_json_threshold_result(self):
        self.set_sensors_cmd()
        self.sensors_cmd[0] += " --json --threshold"
        data = run_shell_cmd(self.sensors_cmd)
        if "not present" in data:
            return {"present": False}
        result = {"present": True}
        result.update(json.loads(data))
        return result
