from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
sys.path.append('../../common')
import unitTestUtil
import logging
from watchdogResetTest import *


CMD_HEADNODE = "sshpass -p password ssh -tt -6 root@{} "
CMD_BMC = "sshpass -p 0penBmc ssh -tt root@{} "

def generateCMD(bmc_ip, headnode_ip=None):
    cmd = CMD_HEADNODE.format(headnode_ip) + CMD_BMC.format(bmc_ip)
    return cmd


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python watchdogResetTest.py type hostname
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['headnode', 'type', 'hostbmc', '--verbose'],
                              [str, str, str, None],
                              ['a ip or name of headnode', 'a platform type', 'a ip or name of hostbmc',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        headnode = args.headnode
        platformType = args.type
        hostname = args.hostbmc
        utilType = util.importUtil(platformType)
        #cmd = CMD_HEADNODE.format(headnode) + CMD_BMC.format(hostname)
        cmd = generateCMD(hostname, headnode)
        watchdogReset(util, utilType, logger, cmd, hostname, headnode)
    except Exception as e:
        print("watchdog Reset Test [FAILED]")
        print("Error: " + e)
    sys.exit(1)
