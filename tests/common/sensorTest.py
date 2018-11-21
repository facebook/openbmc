#!/usr/bin/env python
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import types
import re
import subprocess
import unitTestUtil
import logging

sensorDict = {}
util_support_map = ['fbttn', 'fbtp', 'lightning', 'minipack', 'fby2', 'yosemite']
multi_host_map = ['fby2', 'yosemite']
lm_sensor_support_map = ['wedge', 'wedge100', 'galaxy100', 'cmm']


def sensorTest(platformType, data, util):
    """
    Check that sensor data is with spec or is just present
    """
    # no drivers present from sensor cmd
    if platformType in util_support_map:
        failed = sensorTestUtil(platformType, data, util)
    # drivers present from sensor cmd
    else:
        failed = sensorTestNetwork(platformType, data, util)
    if len(failed) == 0:
        print("Sensor Readings on " + platformType + " [PASSED]")
        sys.exit(0)
    else:
        print("Sensor Readings on " + platformType + " for keys: " +
              str(failed) + " [FAILED]")
        sys.exit(1)


def sensorTestNetwork(platformType, data, util):
    failed = []
    createSensorDictNetworkLmSensors(util)
    logger.debug("Checking values against json file")
    for driver in data:
        if isinstance(data[driver], dict):
            for reading in data[driver]:
                if data[driver][reading] == "yes":
                    try:
                        raw_value = sensorDict[driver][reading]
                    except Exception:
                        failed += [driver, reading]
                        continue
                    if isinstance(data[driver][reading], list):
                        values = re.findall(r"[-+]?\d*\.\d+|\d+", raw_value)
                        if len(values) == 0:
                            failed += [driver, reading]
                            continue
                        rang = data[driver][reading]
                        if float(rang[0]) > float(values[0]) or float(
                                values[0]) > float(rang[1]):
                            failed += [driver, reading]
                    else:
                        if bool(re.search(r'\d', raw_value)):
                            continue
                        else:
                            failed += [driver, reading]
    return failed


def sensorTestUtil(platformType, data, util):
    failed = []
    createSensorDictUtil(util)
    logger.debug("checking values against json file")
    for sensor in data:
        # skip type argument in json file
        if sensor == "type":
            continue
        try:
            raw_values = sensorDict[sensor]
        except Exception:
            failed += [sensor]
            continue
        if platformType in multi_host_map:
            if len(raw_values) not in [1, 4]:
                failed += [sensor]
                continue
        elif len(raw_values) not in [1]:
            failed += [sensor]
            continue
        if isinstance(data[sensor], list):
            for raw_value in raw_values:
                values = re.findall(r"[-+]?\d*\.\d+|\d+", raw_value)
                if len(values) == 0:
                    failed += [sensor]
                    continue
                rang = data[sensor]
                if float(rang[0]) > float(values[0]) or float(values[0]) > float(
                    rang[1]):
                    failed += [sensor]
        else:
            for raw_value in raw_values:
                if 'ok' not in raw_value:
                    failed += [sensor + raw_value]
                    break
    return failed


def createSensorDictNetworkLmSensors(util):
    """
    Creating a sensor dictionary driver -> sensor -> reading
    Supports wedge, wedge100, galaxy100, and cmm
    """
    cmd = util.SensorCmd
    if cmd is None:
        raise Exception("sensor command not implemented")
    logger.debug("executing command: ".format(cmd))
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    logger.debug("Creating sensor dictionary")
    info = info.decode('utf-8')
    info = info.split('\n')
    currentKey = ''
    for line in info:
        if ':' in line:
            lineInfo = line.split(':')
            key = lineInfo[0]
            val = ''.join(lineInfo[1:])
            sensorDict[currentKey][key] = val
        elif len(line) == 0 or line[0] == ' ':
            continue
        else:
            sensorDict[line] = {}
            currentKey = line


def createSensorDictUtil(util):
    """
    Creating a sensor dictionary sensor -> reading
    Supports fbtp and fbttn
    """
    cmd = util.SensorCmd
    if cmd is None:
        raise Exception("sensor command not implemented")
    logger.debug("executing command: " + str(cmd))
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    logger.debug("creating sensor dictionary")
    info = info.decode('utf-8')
    info = info.split('\n')
    for line in info:
        if ':' in line:
            lineInfo = line.split(':')
            key = lineInfo[0]
            val = ''.join(lineInfo[1:])
            if key not in sensorDict:
                sensorDict[key] = []
            sensorDict[key].append(val)
        if "timed out" in line:
            print(line)
            raise Exception(line)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python sensorTest.py wedgeSensor.json
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        data = {}
        args = util.Argparser(['json', '--verbose'], [str, None],
                              ['json file',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        data = util.JSONparser(args.json)
        platformType = data['type']
        utilType = util.importUtil(platformType)
        sensorTest(platformType, data, utilType)
    except Exception as e:
        print("Sensor test [FAILED]")
        print("Error code returned: {}".format(e))
    sys.exit(1)
