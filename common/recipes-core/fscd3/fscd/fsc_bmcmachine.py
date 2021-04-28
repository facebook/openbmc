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
import re
import json
from collections import namedtuple

from fsc_sensor import (
    FscSensorSourceSysfs,
    FscSensorSourceUtil,
    FscSensorSourceKv,
    FscSensorSourceJson,
)
from fsc_util import Logger


SensorValue = namedtuple(
    "SensorValue",
    [
        "id",
        "name",
        "value",
        "unit",
        "status",
        "read_fail_counter",
        "wrong_read_counter",
    ],
)


class BMCMachine(object):
    """
    A container class that can perform sensor read and write.
    """

    def __init__(self):
        self.frus = set()
        self.nums = {}
        self.extra_sensors = {}
        self.last_fan_speed = 0

    def read_sensors(self, sensor_sources, inf):
        """
        Method to read all sensors

        Arguments:
            sensor_sources: Set of all sensor souces from fsc config

        Returns:
            SensorValue tuples
        """
        sensors = {}
        for fru in self.frus:
            sensors[fru] = get_sensor_tuples(fru, self.nums[fru], sensor_sources, inf)

        # read specific sensors
        if self.extra_sensors != {}:
            for sensor_name in self.extra_sensors.keys():
                fru = self.extra_sensors[sensor_name].fru
                sensor = get_sensor_tuples(
                    fru, None, {sensor_name: self.extra_sensors[sensor_name]}, None
                )
                # Merge sensors of the same fru
                if fru in self.frus:
                    sensors[fru] = {
                        **sensors[fru],
                        **sensor,
                    }
                else:
                    sensors[fru] = sensor

        Logger.debug("Last fan speed : %d" % self.last_fan_speed)
        Logger.debug("Sensor reading")

        # Offset the sensor Temp value
        for key, data in list(sensor_sources.items()):
            sensorname = key.lower()

            for fru in self.frus:
                if sensorname in sensors[fru]:
                    senvalue = sensors[fru][sensorname]
                    Logger.debug(" {} = {}".format(sensorname, senvalue.value))

            offset = 0
            if data.offset != None:
                offset = data.offset
            elif data.offset_table != None:
                # Offset sensor Temp, relate with current fan speed
                for (fan_speed, offset_temp) in sorted(data.offset_table):
                    if self.last_fan_speed > fan_speed:
                        offset = offset_temp
                    else:
                        break
            if offset != 0:
                for fru in self.frus:
                    if sensorname in sensors[fru]:
                        senvalue = sensors[fru][sensorname]
                        # Skip sensor if the reading fail
                        if senvalue.value is None:
                            continue
                        value = senvalue.value + offset
                        sensors[fru][sensorname] = senvalue._replace(value=value)
                        value = senvalue.value + offset
                        Logger.debug(
                            " %s = %.2f (after offset %.2f)"
                            % (sensorname, value, offset)
                        )
        return sensors

    def read_fans(self, fans):
        """
        Method to read all fans speeds

        Arguments:
            fans: Set of all sensor fan souces from fsc config

        Returns:
            Fan speeds set
        """
        Logger.debug("Read all fan speeds")
        result = {}
        for key, value in list(fans.items()):
            if isinstance(value.source, FscSensorSourceUtil):
                result[fans[key]] = parse_fan_util(fans[key].source.read())
            elif isinstance(fans[key].source, FscSensorSourceSysfs):
                result[fans[key]] = parse_fan_sysfs(fans[key].source.read())
            elif isinstance(fans[key].source, FscSensorSourceKv):
                result[fans[key]] = parse_fan_kv(fans[key].source.read())
            else:
                Logger.crit("Unknown source type")
        return result

    def set_pwm(self, fan, pct):
        """
        Method to set fan pwm

        Arguments:
            fan: fan sensor object
            pct: new pct to set to the specific fan

        Returns:
            N/A
        """
        Logger.debug("Set pwm %d to %d" % (int(fan.source.name), pct))
        fan.source.write(pct)
        self.last_fan_speed = pct

    def set_all_pwm(self, fans, pct):
        """
        Method to set all fans pwm

        Arguments:
            fans: fan sensor objects
            pct: new pct to set to the specific fan

        Returns:
            N/A
        """
        Logger.debug("Set all pwm to %d" % (pct))
        for key, _value in list(fans.items()):
            self.set_pwm(fans[key], pct)


def get_sensor_tuples(fru_name, sensor_num, sensor_sources, inf):
    """
    Method to walk through each of the sensor sources to build the tuples
    of the form 'SensorValue'

    Arguments:
        fru_name: fru where the sensors should be read from
        sensor_sources: Set of all sensor souces from fsc config
    Returns:
        SensorValue tuples
    """
    result = {}
    for key, value in list(sensor_sources.items()):
        if isinstance(value.source, FscSensorSourceUtil):
            result = parse_all_sensors_util(
                sensor_sources[key].source.read(fru=fru_name, num=sensor_num, inf=inf)
            )
            break  # Hack: util reads all sensors
        elif isinstance(sensor_sources.get(key).source, FscSensorSourceSysfs):
            symbolized_key, tuple = get_sensor_tuple_sysfs(
                key,
                sensor_sources[key].source.read(),
                sensor_sources[key].source.read_source_fail_counter,
                sensor_sources[key].source.read_source_wrong_counter,
            )
            result[symbolized_key] = tuple
        elif isinstance(value.source, FscSensorSourceJson):
            # result += result
            result = {
                **result,
                **parse_sensor_json(
                    sensor_sources[key].source.read(), sensor_sources[key].source
                ),
            }
        else:
            Logger.crit("Unknown source type")
    return result


def get_sensor_tuple_sysfs(key, sensor_data, read_fail_counter=0, wrong_read_counter=0):
    """
    Build a sensor tuple from sensor key and data read

    Arguments:
        sensor_data: data read from sysfs

    Returns:
        SensorValue tuple
    """
    if sensor_data:
        data = int(sensor_data.split("\n")[-2]) / 1000
    else:
        data = -1

    return (
        symbolize_sensorname_sysfs(key),
        SensorValue(
            None, str(key), data, None, None, read_fail_counter, wrong_read_counter
        ),
    )


def symbolize_sensorname_sysfs(name):
    """
    Helper method to normalize the sensor name
    Eg : userver -> userver_temp
    """
    return name.split("_")[1] + "_temp"


def parse_all_sensors_util(sensor_data):
    """
    Parses all sensors data from util

    Arguments:
        sensor_data: blob of sendor data read from util provided

    Returns:
        SensorValue tuples
    """
    result = {}

    sdata = sensor_data.split("\n")
    for line in sdata:
        # skip lines with " or startin with FRU
        if line.find("bic_read_sensor_wrapper") != -1:
            continue
        if line.find("failed") != -1:
            continue
        # NA case match
        if line.find(" NA ") != -1:
            m = re.match(r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+.\s+\((.+)\)$", line)
            if m is not None:
                sid = int(m.group(2), 16)
                name = m.group(1).strip()
                value = None
                status = m.group(4)
                symname = symbolize_sensorname(name)
                result[symname] = SensorValue(sid, name, value, None, status, 0, 0)
            continue
        # normal case match
        m = re.match(
            r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+([^\s]+)\s+.\s+\((.+)\)$", line
        )
        if m is not None:
            sid = int(m.group(2), 16)
            name = m.group(1).strip()
            value = float(m.group(3))
            unit = m.group(4)
            status = m.group(5)
            symname = symbolize_sensorname(name)
            result[symname] = SensorValue(sid, name, value, unit, status, 0, 0)
    return result


def parse_sensor_json(raw_data, source):
    result = {}
    max_value = None
    filter = source.get_filter()
    try:
        obj = json.loads(raw_data)
        for sensor in obj["data"]:
            if filter is not None:
                m = re.match(filter, sensor)
                if m is None:  # skip the sensor has name not match in regex
                    continue
            value = obj["data"][sensor]["value"]
            if max_value is None or max_value < value:
                max_value = value
        result["json_max_temp"] = SensorValue(
            None, "JSON_MAX_TEMP", max_value, "C", None, 0, 0
        )
    except Exception as e:
        Logger.crit(
            "Exception while trying to decode JSON {raw_data}: {exp}".format(
                raw_data=repr(raw_data), exp=repr(e)
            )
        )
    return result


def symbolize_sensorname(name):
    """
    Helper method to normalize the sensor name
    Eg : SOC Therm Margin -> soc_therm_margin
    """
    return name.lower().replace(" ", "_")


def parse_fan_sysfs(sensor_data):
    """
    Parse the data read from sysfs and return the PWM

    Arguments:
        sensor_data: data read from sysfs

    Returns:
        PWM
    """
    if sensor_data:
        return int(sensor_data)
    else:
        return -1


def parse_fan_util(fan_data):
    """
    Parses fan data from util

    Arguments:
        fan_data: blob of fan speed data read from util provided

    Returns:
        Fan speeds set
    """
    sdata = fan_data.split("\n")
    for line in sdata:
        m = re.match(r"Fan .*\sSpeed:\s+(\d+)\s", line)
        if m is not None:
            return int(m.group(1))
    return -1


def parse_fan_kv(sensor_data):
    """
    Parse the data read from kv and return the RPM

    Arguments:
        sensor_data: data read from kv

    Returns:
        RPM
    """
    if sensor_data and sensor_data != "NA":
        return int(float(sensor_data))
    else:
        return -1
