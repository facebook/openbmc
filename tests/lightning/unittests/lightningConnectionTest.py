from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
sys.path.append('../../common')
import unitTestUtil
import logging
from connectionTest import pingTest

CMD_JUMPHOST = "sshpass -p password ssh -tt -6 root@{}"

CMD = "ping -c 1 -q -w 1 {}"
CMD6 = "ping6 -c 1 -q -w 1 {}"
PING_CMDS = {4: "ping -c 1 -q -w 1 {}", 6: "ping6 -c 1 -q -w 1 {}"}

def pingCMD(version, headnodeName, ping):
    pingcmd = CMD_JUMPHOST.format(headnodeName) + ' ' + PING_CMDS[version]
    return pingcmd


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python connectionTest.py hostname version
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['headnode', 'ping', 'version', '--verbose'], [
            str, str, int, None
        ], [
            'host name or IP address of headnode for lightning',
            'a host name or IP address', 'a version number',
            'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'
        ])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        headnodeName = args.headnode
        ping = args.ping
        version = args.version
        pingcmd = pingCMD(version, headnodeName, ping)
        version = str(version)
        pingTest(ping, version, logger, pingcmd)
    except Exception as e:
        print("Ping Test [FAILED]")
        print("Error: " + str(e))
        sys.exit(1)
