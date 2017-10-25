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
util_support_map = ['fbttn', 'fbtp', 'lightning']
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
            raw_value = sensorDict[sensor]
        except Exception:
            failed += [sensor]
            continue
        if isinstance(data[sensor], list):
            values = re.findall(r"[-+]?\d*\.\d+|\d+", raw_value)
            if len(values) == 0:
                failed += [sensor]
                continue
            rang = data[sensor]
            if float(rang[0]) > float(values[0]) or float(values[0]) > float(
                    rang[1]):
                failed += [sensor]
        else:
            if 'NA' in raw_value:
                failed += [sensor]
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
            sensorDict[key] = val


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
