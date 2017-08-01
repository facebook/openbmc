from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import pxssh
import sys
import subprocess
import pexpect
import os
sys.path.insert(0, '../')
import unitTestUtil


VERBOSE = False
MODE = 'WARN'


def setVerbose(cmd):
    return cmd + ' --verbose ' + MODE


def interfaceTest(ssh, data):
    """
    Run interfaceTest.py with command line arguments
    """
    if data["interfaceTest.py"][1]["eth0"]["v4"] == "yes":
        args = "eth0 4"
        if VERBOSE:
            args = setVerbose(args)
        ssh.sendline('python interfaceTest.py ' + args)
        ssh.prompt(timeout=100)
        output = ssh.before
        print(output.split('\n', 1)[1])
    if data["interfaceTest.py"][1]["eth0"]["v6"] == "yes":
        args = "eth0 6"
        if VERBOSE:
            args = setVerbose(args)
        ssh.sendline('python interfaceTest.py ' + args)
        ssh.prompt(timeout=100)
        output = ssh.before
        print(output.split('\n', 1)[1])
    if data["interfaceTest.py"][1]["usb0"] == "yes":
        args = "usb0 6"
        if VERBOSE:
            args = setVerbose(args)
        ssh.sendline('python interfaceTest.py ' + args)
        ssh.prompt(timeout=100)
        output = ssh.before
        print(output.split('\n', 1)[1])
    if data["interfaceTest.py"][1]["eth0.4088"] == "yes":
        args = "eth0.4088 6"
        if VERBOSE:
            args = setVerbose(args)
        ssh.sendline('python interfaceTest.py ' + args)
        ssh.prompt(timeout=100)
        output = ssh.before
        print(output.split('\n', 1)[1])
    return


def generalTypeTest(ssh, data, testName):
    """
    Run testName.py with command line arguments
    """
    platformType = data["type"]
    cmd = 'python ' + testName + ' ' + platformType
    if VERBOSE:
        cmd = setVerbose(cmd)
    ssh.sendline(cmd)
    ssh.prompt(timeout=1000)
    output = ssh.before
    print(output.split('\n', 1)[1])
    return


def generalJsonTest(ssh, data, testName):
    json = data[testName][1]['json']
    cmd = 'python ' + testName + ' ' + json
    if VERBOSE:
        cmd = setVerbose(cmd)
    ssh.sendline(cmd)
    ssh.prompt(timeout=1000)
    output = ssh.before
    print(output.split('\n', 1)[1])
    return


def connectionTest(hostnameBMC):
    version = data['connectionTest.py'][1]['version']
    cmd = 'python ../connectionTest.py ' + str(hostnameBMC) + ' ' + str(version)
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info)
    return


def memoryUsageTest(ssh, data):
    threshold = data['memoryUsageTest.py'][1]['threshold']
    cmd = 'python memoryUsageTest.py ' + threshold
    if VERBOSE:
        cmd = setVerbose(cmd)
    ssh.sendline(cmd)
    ssh.prompt(timeout=1000)
    output = ssh.before
    print(output.split('\n', 1)[1])
    return


def generalTypeBMCTest(ssh, data, testName, BMC):
    platformType = data["type"]
    cmd = 'python ../' + testName + ' ' + platformType + ' ' + hostnameBMC
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info)
    return


def powerCycleSWTest(ssh, data, hostnameBMC, hostnameMS):
    platformType = data["type"]
    cmd = 'python ../powerCycleSWTest.py ' + platformType + ' ' + hostnameBMC + ' ' + hostnameMS
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info)
    return

def cmmComponentPresenceTest(ssh, data, testName):
    cmd = "python " + testName
    if VERBOSE:
        cmd = setVerbose(cmd)
    ssh.sendline(cmd)
    ssh.prompt(timeout=1000)
    output = ssh.before
    print(output.split('\n', 1)[1])
    return


if __name__ == "__main__":
    """
    Commamd line interface with an input of a json file.
    """
    # parse json string
    data = {}
    util = unitTestUtil.UnitTestUtil()
    try:
        args = util.Argparser(['json', 'hostnameBMC', 'hostnameMS', '--verbose'],
                              [str, str, str, None],
                              ['a json file', 'a hostname for BMC',
                              'a hostname for the Micro server',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        json = args.json
        if args.verbose is not None:
            VERBOSE = True
            MODE = args.verbose
        hostnameBMC = args.hostnameBMC
        hostnameMS = args.hostnameMS
        data = util.JSONparser(json)
    except Exception as e:
        print("Error returned: " + str(e))

    if len(data) != 0:
        # scp directory
        path = os.getcwd().replace('/common/tools', '')
        util.scp(path, hostnameBMC, True, pexpect)

        # login to host
        ssh = pxssh.pxssh()
        util.Login(hostnameBMC, ssh)
        ssh.sendline('cd /tmp/tests/common')
        ssh.prompt()

        # run tests
        if "cmmComponentPresenceTest.py" in data:
            if data["cmmComponentPresenceTest.py"] == 'yes':
                cmmComponentPresenceTest(ssh, data, "cmmComponentPresenceTest.py")
        if "cmmPowerGPIOPresence.py" in data:
            if data["cmmPowerGPIOPresence.py"] == 'yes':
                cmmComponentPresenceTest(ssh, data, "cmmPowerGPIOPresence.py")
        if "interfaceTest.py" in data:
            if data["interfaceTest.py"][0] == 'yes':
                interfaceTest(ssh, data)
        if "eepromTest.py" in data:
            if data["eepromTest.py"] == 'yes':
                generalTypeTest(ssh, data, "eepromTest.py")
        if "fansTest.py" in data:
            if data["fansTest.py"] == 'yes':
                generalTypeTest(ssh, data, "fansTest.py")
        if "connectionTest.py" in data:
            if data["connectionTest.py"][0] == 'yes':
                connectionTest(hostnameBMC)
        if "sensorTest.py" in data:
            if data["sensorTest.py"][0] == 'yes':
                generalJsonTest(ssh, data, "sensorTest.py")
        if "i2cSensorTest.py" in data:
            if data["i2cSensorTest.py"][0] == 'yes':
                generalJsonTest(ssh, data, "i2cSensorTest.py")
        if "memoryUsageTest.py" in data:
            if data["memoryUsageTest.py"][0] == 'yes':
                memoryUsageTest(ssh, data)
        if "kernelModulesTest.py" in data:
            if data["kernelModulesTest.py"][0] == 'yes':
                generalJsonTest(ssh, data, "kernelModulesTest.py")
        if "processRunningTest.py" in data:
            if data["processRunningTest.py"][0] == 'yes':
                generalJsonTest(ssh, data, "processRunningTest.py")
        if "solTest.py" in data:
            if data["solTest.py"] == 'yes':
                generalTypeBMCTest(ssh, data, "solTest.py", hostnameBMC)
        if "powerCycleHWTest.py" in data:
            if data["powerCycleHWTest.py"] == 'yes':
                generalTypeTest(ssh, data, "powerCycleHWTest.py")
        if "powerCycleSWTest.py" in data:
            if data["powerCycleSWTest.py"] == 'yes':
                powerCycleSWTest(ssh, data, hostnameBMC, hostnameMS)
        if "watchdogResetTest.py" in data:
            if data["watchdogResetTest.py"] == 'yes':
                generalTypeBMCTest(ssh, data, "watchdogResetTest.py", hostnameBMC)
