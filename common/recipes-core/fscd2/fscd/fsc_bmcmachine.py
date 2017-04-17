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
from collections import namedtuple
from fsc_sensor import FscSensorSourceUtil
from fsc_util import Logger

SensorValue = namedtuple(
                'SensorValue',
                ['id',
                 'name',
                 'value',
                 'unit',
                 'status'])


class BMCMachine(object):
    '''
    A container class that can perform sensor read and write.
    '''
    def __init__(self):
        self.frus = set()

    def read_sensors(self, sensor_sources):
        '''
        Method to read all sensors

        Arguments:
            sensor_sources: Set of all sensor souces from fsc config

        Returns:
            SensorValue tuples
        '''
        sensors = {}
        for fru in self.frus:
            sensors[fru] = get_sensor_tuples(fru, sensor_sources)
        return sensors

    def read_fans(self, fans):
        '''
        Method to read all fans speeds

        Arguments:
            fans: Set of all sensor fan souces from fsc config

        Returns:
            Fan speeds set
        '''
        Logger.debug("Read all fan speeds")
        result = {}
        for key, value in fans.items():
            if isinstance(fans[key].source, FscSensorSourceUtil):
                result = parse_all_fans_util(
                    fans[key].source.read())
                break  # Hack: util will read all fans
            else:
                Logger.crit("Unknown source type")
        return result

    def set_pwm(self, fan, pct):
        '''
        Method to set fan pwm

        Arguments:
            fan: fan sensor object
            pct: new pct to set to the specific fan

        Returns:
            N/A
        '''
        Logger.debug("Set pwm %d to %d" % (int(fan.source.name), pct))
        fan.source.write(pct)

    def set_all_pwm(self, fans, pct):
        '''
        Method to set all fans pwm

        Arguments:
            fans: fan sensor objects
            pct: new pct to set to the specific fan

        Returns:
            N/A
        '''
        Logger.debug("Set all pwm to %d" % (pct))
        for key, value in fans.items():
            self.set_pwm(fans[key], pct)

def get_sensor_tuples(fru_name, sensor_sources):
    '''
    Method to walk through each of the sensor sources to build the tuples
    of the form 'SensorValue'

    Arguments:
        fru_name: fru where the sensors should be read from
        sensor_sources: Set of all sensor souces from fsc config
    Returns:
        SensorValue tuples
    '''
    result = {}
    for key, value in sensor_sources.items():
        if isinstance(sensor_sources.get(key).source, FscSensorSourceUtil):
            result = parse_all_sensors_util(
                sensor_sources[key].source.read(fru=fru_name))
            break  # Hack: util reads all sensors
        else:
            Logger.crit("Unknown source type")
    return result


def parse_all_sensors_util(sensor_data):
    '''
    Parses all sensors data from util

    Arguments:
        sensor_data: blob of sendor data read from util provided

    Returns:
        SensorValue tuples
    '''
    result = {}

    sdata = sensor_data.split('\n')
    for line in sdata:
        # skip lines with " or startin with FRU
        if line.find("bic_read_sensor_wrapper") != -1:
            continue
        if line.find("failed") != -1:
            continue

        if line.find(" NA "):
            m = re.match(r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+.\s+\((.+)\)$", line)
            if m is not None:
                sid = int(m.group(2), 16)
                name = m.group(1).strip()
                value = None
                status = m.group(4)
                symname = symbolize_sensorname(name)
                result[symname] = SensorValue(sid,
                                              name,
                                              value,
                                              None,
                                              status)
                continue
        m = re.match(r"^(.*)\((0x..?)\)\s+:\s+([^\s]+)\s+([^\s]+)\s+.\s+\((.+)\)$", line)
        if m is not None:
            sid = int(m.group(2), 16)
            name = m.group(1).strip()
            value = float(m.group(3))
            unit = m.group(4)
            status = m.group(5)
            symname = symbolize_sensorname(name)
            result[symname] = SensorValue(sid, name, value, unit, status)
    return result


def symbolize_sensorname(name):
    '''
    Helper method to normalize the sensor name
    Eg : SOC Therm Margin -> soc_therm_margin
    '''
    return name.lower().replace(" ", "_")

def parse_all_fans_util(fan_data):
    '''
    Parses all fan data from util

    Arguments:
        fan_data: blob of fan speed data read from util provided

    Returns:
        Fan speeds set
    '''
    sdata = fan_data.split('\n')
    result = {}
    for line in sdata:
        m = re.match(r"Fan (\d+).*\sSpeed:\s+(\d+)\s", line)
        if m is not None:
            result[int(m.group(1))] = int(m.group(2))
    return result
