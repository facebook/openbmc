from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import unitTestUtil
import pxssh
import time
import subprocess
import os
import logging
currentPath = os.getcwd()
try:
    testPath = currentPath[0:currentPath.index('common')]
except Exception:
    testPath = '/tmp/tests/'


def watchdogReset(unitTestUtil, utilType, hostname):
    """
    Test that watchdog is working by causing a reset
    """
    if utilType.daemonProcessesKill is None:
        raise Exception("no daemon process kill commands listed")
    # Login and run WatchdogProcess
    logger.debug("starting watchdog process from this test")
    ssh = pxssh.pxssh()
    unitTestUtil.Login(hostname, ssh)
    ssh.sendline('cd /tmp/tests/common/')
    ssh.prompt()
    ssh.sendline('python watchdogProcess.py')
    ssh.prompt(timeout=5)  # process will be running constantly

    # Login and kill daemon processes
    logger.debug("killing all daemon processes")
    ssh2 = pxssh.pxssh()
    unitTestUtil.Login(hostname, ssh2)
    killcmds = utilType.daemonProcessesKill
    for cmd in killcmds:
        ssh2.sendline(cmd)
        ssh2.prompt()
    # kill process that just started from this script
    logger.debug("killing watchdog process started from this test")
    ssh2.sendline('ps')
    ssh2.prompt()
    info = ssh2.before
    info = info.split('\n')
    for line in info:
        if "watchdogProcess" in line:
            line = line.split(' ')
            process = ''
            for word in line:
                if word != '':
                    process = word
                    break
            ssh2.sendline('kill ' + process)
            ssh2.prompt()
    # check that host is pingable -> not pingable -> pingable
    pingcmd = ['python', testPath + 'common/connectionTest.py', hostname, '6']
    logger.debug("checking that ping connection to host succeeds before reboot is triggered")
    f = subprocess.Popen(pingcmd,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if 'PASSED' not in out:
        print("Watchdog Reset Test [FAILED]")
        sys.exit(1)
    logger.debug("waiting 30 seconds")
    time.sleep(30)
    logger.debug("checking that ping connection to host fails because rebooting is occurring")
    f = subprocess.Popen(pingcmd,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if 'PASSED' in out:
        print("No reset from watchdog [FAILED]")
        sys.exit(1)
    logger.debug("waiting 240 seconds")
    time.sleep(240)
    logger.debug("checking that ping connection to host succeeds, reboot should have completed")
    f = subprocess.Popen(pingcmd,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if 'PASSED' not in out:
        print("Watchdog Reset Test [FAILED]")
        sys.exit(1)
    else:
        print("Watchdog Reset Test [PASSED]")
        sys.exit(0)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python watchdogResetTest.py type hostname
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['type', 'host', '--verbose'], [str, str, None],
                              ['a platform type', 'a hostname',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        platformType = args.type
        hostname = args.host
        utilType = util.importUtil(platformType)
        watchdogReset(util, utilType, hostname)
    except Exception as e:
        print("watchdog Reset Test [FAILED]")
        print("Error: " + str(e))
    sys.exit(1)
