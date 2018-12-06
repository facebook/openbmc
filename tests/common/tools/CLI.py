from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
from subprocess import *
try:
    from pexpect import pxssh
except:
    import pxssh
import sys
import subprocess
import pexpect
import os
sys.path.insert(0, '../')
import unitTestUtil


VERBOSE = False
MODE = 'WARN'
HEADNODE = False

def setVerbose(cmd):
    return cmd + ' --verbose ' + MODE


def interfaceTest(cmd_bmc, data):
    """
    Run interfaceTest.py with command line arguments
    """
    if data["interfaceTest.py"][1]["eth0"]["v4"] == "yes":
        args = "eth0 4"
        if VERBOSE:
            args = setVerbose(args)
        cmd = cmd_bmc + 'python /tmp/tests/common/interfaceTest.py ' + args
        sshProcess = Popen(
            cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
        output = sshProcess.communicate()
        if VERBOSE:
            print(output[0].decode())
        else:
            print(output[0].decode().split('\n')[0].rstrip())
    if data["interfaceTest.py"][1]["eth0"]["v6"] == "yes":
        args = "eth0 6"
        if VERBOSE:
            args = setVerbose(args)
        cmd = cmd_bmc + 'python /tmp/tests/common/interfaceTest.py ' + args
        sshProcess = Popen(
            cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
        output = sshProcess.communicate()
        if VERBOSE:
            print(output[0].decode())
        else:
            print(output[0].decode().split('\n')[0].rstrip())
    if data["interfaceTest.py"][1]["usb0"] == "yes":
        args = "usb0 6"
        if VERBOSE:
            args = setVerbose(args)
        cmd = cmd_bmc + 'python /tmp/tests/common/interfaceTest.py ' + args
        sshProcess = Popen(
            cmd,
            shell=True,
            stdin=None,
            stdout=PIPE,
            stderr=PIPE,
            universal_newlines=True)
        output = sshProcess.communicate()
        # in python 3, communicate has the timeout parameter
        # output = sshProcess.communicate(timeout=100)
        if VERBOSE:
            print(output[0])
        else:
            print(output[0].rstrip().split('\n')[0])
    if data["interfaceTest.py"][1]["eth0.4088"] == "yes":
        args = "eth0.4088 6"
        if VERBOSE:
            args = setVerbose(args)
        cmd = cmd_bmc + 'python /tmp/tests/common/interfaceTest.py ' + args
        sshProcess = Popen(
            cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
        output = sshProcess.communicate()
        print(output[0].rstrip())
    return


def generalTypeTest(cmd_bmc, data, testName, testPath="/common/"):
    """
    Run testName.py with command line arguments on the target BMC
    """
    platformType = data["type"]
    cmd = cmd_bmc + 'python /tmp/tests' + testPath + testName + ' ' + platformType
    if VERBOSE:
        cmd = setVerbose(cmd)
    sshProcess = Popen(cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
    output = sshProcess.communicate()
    if VERBOSE:
        print(output[0].decode())
    else:
        print(output[0].decode().split('\n')[0].rstrip())
    return

def platformOnlyTest(cmd_bmc, data, testName, testPath="/common/"):
    """
    Run testName.py with command line arguments on the target BMC
    """
    cmd = cmd_bmc + 'python /tmp/tests' + testPath + testName
    sshProcess = Popen(cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
    output = sshProcess.communicate()
    print(output[0].decode())
    return

def generalJsonTest(cmd_bmc, data, testName):
    json = data[testName][1]['json']
    cmd = "python /tmp/tests/common/{0} /tmp/tests/common/{1}"
    cmd = cmd_bmc + cmd.format(str(testName), json)
    if VERBOSE:
        cmd = setVerbose(cmd)
    sshProcess = Popen(cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
    output = sshProcess.communicate()
    if VERBOSE:
        print(output[0].decode())
    else:
        print(output[0].decode().split('\n')[0].rstrip())
    return


def connectionTest(hostnameBMC, data, headnodeName=None):
    version = data['connectionTest.py'][1]['version']
    cmd = ""
    if HEADNODE:
        pythonfile = data['connectionTest.py'][1]['testfile']
        cmd = "python {0} {1} {2} {3}"
        cmd = cmd.format(str(pythonfile), str(headnodeName), str(hostnameBMC), str(version))
    else:
        cmd = "python ../connectionTest.py {0} {1}"
        cmd = cmd.format(str(hostnameBMC), str(version))
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info.decode().rstrip())
    return

def restapiTest(hostnameBMC, data, headnodeName=None):
    json = data["restapiTest.py"][1]['json']
    if HEADNODE:
        pythonfile = data["restapiTest.py"][1]['testfile']
        cmd = "python3 {0} {1} {2}"
        cmd = cmd.format(pythonfile, headnodeName, json)
    else:
        cmd = "python3 ../restapiTest.py {0} {1}"
        cmd = cmd.format(str(hostnameBMC), str(json))
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info.decode().rstrip())
    return

def memoryUsageTest(cmd_bmc, data):
    threshold = data['memoryUsageTest.py'][1]['threshold']
    cmd = cmd_bmc + 'python /tmp/tests/common/memoryUsageTest.py ' + threshold
    if VERBOSE:
        cmd = setVerbose(cmd)
    sshProcess = Popen(cmd, shell=True, stdin=None, stdout=PIPE, stderr=PIPE)
    output = sshProcess.communicate()
    print(output[0].decode().split('\n')[0].rstrip())
    return


def generalTypeBMCTest(hostnameBMC, testName, data, headnodeName=None):
    platformType = data["type"]
    cmd = ""
    if HEADNODE:
        pythonfile = data[testName][1]['testfile']
        cmd = "python {0} {1} {2} {3}"
        cmd = cmd.format(pythonfile, headnodeName, platformType, hostnameBMC)
    else:
        cmd = "python ../{0} {1} {2}"
        cmd = cmd.format(testName, platformType, hostnameBMC)
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info)
    return


def powerCycleSWTest(cmd_bmc, data, hostnameBMC, hostnameMS):
    platformType = data["type"]
    cmd = 'python ../powerCycleSWTest.py {0} {1} {2}'
    cmd = cmd.format(platformType, hostnameBMC, hostnameMS)
    if VERBOSE:
        cmd = setVerbose(cmd)
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    print(info)
    if len(info) == 0:
        print(err)
    return


def cmmComponentPresenceTest(ssh, data, testName):
    cmd = "python " + testName
    if VERBOSE:
        cmd = setVerbose(cmd)
    ssh.sendline(cmd)
    ssh.prompt(timeout=1000)
    output = ssh.before
    print(str(output).split('\n', 1)[1])
    return


if __name__ == "__main__":
    """
    Commamd line interface with an input of a json file.
    """
    # parse json string
    data = {}
    cmd_headnode = "sshpass -p password ssh -tt -6 root@{} "
    cmd_bmc = "sshpass -p 0penBmc ssh -tt root@{} "
    util = unitTestUtil.UnitTestUtil()
    try:
        args = util.Argparser(['json', '--headnode', 'hostnameBMC', 'hostnameMS', '--verbose', '--skip_scp'],
                              [str, None, str, str, None, None],
                              ['Test json file',
                               'Hostname for headnode',
                               'Hostname for BMC',
                               'Hostname for  host',
                               'Debug option: DEBUG, INFO, WARNING, ERROR',
                               'Skip copying tests to target. If set user should have copied tests manually. Usage "--skip_scp True" '],
                              [False, False, True, False, False, False])
        hostnameBMC = ""
        headnodeName = ""
        json = args.json
        hostnameMS = args.hostnameMS
        if args.hostnameBMC is not None:
            hostnameBMC = args.hostnameBMC
        else:
            index = hostnameMS.index('.')
            hostnameBMC = hostnameMS[:index] + '-oob' + hostnameMS[index:]
        if args.headnode is not None:
            HEADNODE = True
            headnodeName = args.headnode
        if args.verbose is not None:
            VERBOSE = True
            MODE = args.verbose
        if HEADNODE:
            cmd_bmc = cmd_headnode.format(headnodeName) + cmd_bmc.format(hostnameBMC)
        else:
            cmd_bmc = cmd_bmc.format(hostnameBMC)
        data = util.JSONparser(json)
    except Exception as e:
        print("Error returned: " + str(e))

    if len(data) != 0:
        # scp directory
        path = os.getcwd().replace('/common/tools', '')
        #util.scp(path, hostnameBMC, True, pexpect)

        # login to host
        ssh = pxssh.pxssh()
        if HEADNODE is False:
            if args.skip_scp is None:
                # scp directory
                util.scp(path, hostnameBMC, True, pexpect)
            # login to host
            util.Login(hostnameBMC, ssh)
            ssh.sendline('cd /tmp/tests/common')
            ssh.prompt()
        else:
            if args.skip_scp is None:
                util.scp_through_proxy(path, headnodeName, hostnameBMC,
                                    True, pexpect)

        # run tests
        if "cmmComponentPresenceTest.py" in data:
            if data["cmmComponentPresenceTest.py"] == 'yes':
                cmmComponentPresenceTest(ssh, data,
                                         "cmmComponentPresenceTest.py")
        if "cmmPowerGPIOPresence.py" in data:
            if data["cmmPowerGPIOPresence.py"] == 'yes':
                cmmComponentPresenceTest(ssh, data, "cmmPowerGPIOPresence.py")
        if "interfaceTest.py" in data:
            if data["interfaceTest.py"][0] == 'yes':
                interfaceTest(cmd_bmc, data)
        if "eepromTest.py" in data:
            if data["eepromTest.py"] == 'yes':
                generalTypeTest(cmd_bmc, data, "eepromTest.py")
        if "hostMacTest.py" in data:
            if data["hostMacTest.py"] == 'yes':
                generalTypeTest(cmd_bmc, data, "hostMacTest.py")
        if "fansTest.py" in data:
            if data["fansTest.py"] == 'yes':
                generalTypeTest(cmd_bmc, data, "fansTest.py")
        if "restapiTest.py" in data:
            if HEADNODE:
                if data["restapiTest.py"][0] == 'yes':
                    restapiTest(hostnameBMC, data, headnodeName)
            else:
                if data["restapiTest.py"][0] == 'yes':
                    restapiTest(hostnameBMC, data)
        if "connectionTest.py" in data:
            if data["connectionTest.py"][0] == 'yes':
                if HEADNODE:
                    connectionTest(hostnameBMC, data, headnodeName)
                else:
                    connectionTest(hostnameBMC, data)
        if "sensorTest.py" in data:
            if data["sensorTest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "sensorTest.py")
        if "i2cSensorTest.py" in data:
            if data["i2cSensorTest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "i2cSensorTest.py")
        if "memoryUsageTest.py" in data:
            if data["memoryUsageTest.py"][0] == 'yes':
                memoryUsageTest(cmd_bmc, data)
        if "kernelModulesTest.py" in data:
            if data["kernelModulesTest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "kernelModulesTest.py")
        if "gpioTest.py" in data:
            if data["gpioTest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "gpioTest.py")
        if "processRunningTest.py" in data:
            if data["processRunningTest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "processRunningTest.py")
        if "bmcUtiltest.py" in data:
            if data["bmcUtiltest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "bmcUtiltest.py")
        if "scriptsDependencyTest.py" in data:
            if data["scriptsDependencyTest.py"][0] == 'yes':
                generalJsonTest(cmd_bmc, data, "scriptsDependencyTest.py")
        if "solTest.py" in data:
            if data["solTest.py"] == 'yes':
                if HEADNODE:
                    generalTypeBMCTest(hostnameBMC, "solTest.py", data,
                                       headnodeName)
                else:
                    generalTypeBMCTest(hostnameBMC, "solTest.py", data)
        # Platform specific tests
        if "fscd_test.py" in data:
            if data["fscd_test.py"][0] == 'yes':
                platformOnlyTest(cmd_bmc, data, "fscd_test.py", data["fscd_test.py"][1]["testfile"])
        if "psumuxmon_test.py" in data:
            if data["psumuxmon_test.py"][0] == 'yes':
                platformOnlyTest(cmd_bmc, data, "psumuxmon_test.py", data["psumuxmon_test.py"][1]["testfile"])

        # Watchdog test.
        if "watchdogResetTest.py" in data:
            if data["watchdogResetTest.py"] == 'yes':
                generalTypeTest(cmd_bmc, data, "watchdogResetTest.py")

        # these tests need to be run at the end because when BMC reboots,
        # tests are lost from the system
        if "powerCycleHWTest.py" in data:
            if data["powerCycleHWTest.py"] == 'yes':
                generalTypeTest(cmd_bmc, data, "powerCycleHWTest.py")
        if "powerCycleSWTest.py" in data:
            if data["powerCycleSWTest.py"] == 'yes':
                powerCycleSWTest(cmd_bmc, data, hostnameBMC, hostnameMS)
