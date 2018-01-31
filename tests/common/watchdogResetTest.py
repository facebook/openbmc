
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import unitTestUtil
import time
import subprocess
import os
import logging
CMD_BMC = "sshpass -p 0penBmc ssh -tt root@{} "
currentPath = os.getcwd()
try:
    testPath = currentPath[0:currentPath.index('tests')]
except Exception:
    testPath = '/tmp/'


def watchdogReset(unitTestUtil, utilType, logger, cmd_bmc, hostname, headnode=None):
    """
    Test that watchdog is working by causing a reset
    """
    if utilType.daemonProcessesKill is None:
        raise Exception("no daemon process kill commands listed")
    # Login and run WatchdogProcess
    logger.debug("starting watchdog process from this test")
    cmd_run_wdp = "python /tmp/tests/common/watchdogProcess.py"
    wdSubProcess = subprocess.Popen(cmd_bmc + cmd_run_wdp,
                                    shell=True, stdin=None,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

    # Login and kill daemon processes
    logger.debug("killing all daemon processes")
    killcmds = utilType.daemonProcessesKill
    for cmd in killcmds:
        killSubProcess = subprocess.Popen(cmd_bmc + cmd,
                                          shell=True, stdin=None,
                                          stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE)
        output = killSubProcess.communicate()
    # kill process that just started from this script
    logger.debug("killing watchdog process started from this test")
    psSubProcess = subprocess.Popen(cmd_bmc + "ps",
                                    shell=True, stdin=None,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
    output = psSubProcess.communicate()[0]
    #info = output.split('\n')
    info = output.decode('utf-8').split('\n')
    for line in info:
        if "watchdogProcess" in line:
            line = line.split(' ')
            for word in line:
                if word != '':
                    killprocess = subprocess.Popen(cmd_bmc + 'kill ' + word,
                                                   shell=True, stdin=None,
                                                   stdout=subprocess.PIPE,
                                                   stderr=subprocess.PIPE)
                    output = killprocess.communicate()
                    break

    # check that host is pingable -> not pingable -> pingable
    pingcmd = ""
    if headnode is None or utilType.ConnectionTestPath is None:
        pingcmd = 'python {0}tests/common/connectionTest.py {1} 6'.format(testPath, hostname)
        #pingcmd = ['python', testPath + 'common/connectionTest.py', hostname, '6']
    else:
        pingcmd = 'python {0}{1} {2} {3} 6'.format(testPath, utilType.ConnectionTestPath, headnode, hostname)
    logger.debug("checking that ping connection to host succeeds before reboot is triggered")
    f = subprocess.Popen(pingcmd,
                         shell=True, stdin=None,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if b'PASSED' not in out:
        print("watchdog Reset Test [FAILED]")
        sys.exit(1)
    logger.debug("waiting 30 seconds")
    time.sleep(30)
    logger.debug("checking that ping connection to host fails because rebooting is occurring")
    f = subprocess.Popen(pingcmd,
                         shell=True, stdin=None,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if b'PASSED' in out:
        print("No reset from watchdog [FAILED]")
        sys.exit(1)
    logger.debug("waiting 900 seconds")
    time.sleep(900)
    logger.debug("checking that ping connection to host succeeds, reboot should have completed")
    f = subprocess.Popen(pingcmd,
                         shell=True, stdin=None,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        print("checking not connected: err")
        print(err)
        raise Exception(err)
    if b'PASSED' not in out:
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
        args = util.Argparser(['type', 'hostbmc', '--verbose'], [str, str, None],
                              ['a platform type', 'a ip or name of hostbmc',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        platformType = args.type
        hostname = args.hostbmc
        utilType = util.importUtil(platformType)
        cmd_bmc = CMD_BMC.format(hostname)
        watchdogReset(util, utilType, logger, cmd_bmc, hostname)
    except Exception as e:
        print("watchdog Reset Test [FAILED]")
        print("Error: " + str(e))
    sys.exit(1)
