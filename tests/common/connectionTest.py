from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import subprocess
import sys
import unitTestUtil
import logging


def pingTest(ping, version):
    """
    Ping a host from outside the platform
    """
    if int(version) == 4:
        logger.debug("executing ipv4 ping command")
        cmd = ["ping", "-c", "1", "-q", "-w", "1", ping]
        f = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        err = f.communicate()[1]
        logger.debug("ping command successfully executed")
        if len(err) > 0:
            raise Exception(err)
    elif int(version) == 6:
        logger.debug("executing ipv6 ping command")
        cmd = ["ping6", "-c", "1", "-q", "-w", "1", ping]
        f = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        err = f.communicate()[1]
        if len(err) > 0:
            raise Exception(err)
        logger.debug("ping6 command successfully executed")
    if f.returncode == 0:
        print('Connection for ' + ping + ' v' + str(version) + " [PASSED]")
        sys.exit(0)
    else:
        print('Connection for ' + ping + ' v' + str(version) + " [FAILED]")
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python connectionTest.py hostname version
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(
            ['ping', 'version', '--verbose'], [str, int, None], [
                'a host name or IP address', 'a version number',
                'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'
            ])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        ping = args.ping
        version = str(args.version)
        pingTest(ping, version)
    except Exception as e:
        print("Ping Test [FAILED]")
        print("Error: " + str(e))
        sys.exit(1)
