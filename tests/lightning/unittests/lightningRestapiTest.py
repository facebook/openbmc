from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
sys.path.append('../../common')
import unitTestUtil
import logging
import subprocess
from restapiTest import *

CMD_HEADNODE = "sshpass -p password ssh -tt -6 root@{} "
LINKLOCAL_ADDR = "fe80::ff:fe00:1%usb0"
CMD_TCP_LISTEN = "/bin/socat -6 tcp-listen:8080,fork tcp:[{}]:8080"
#CMD_RUN_REST = "sshpass -p 0penBmc ssh -tt root@{} python /usr/local/bin/rest.py"


def generateCMD(icmd, argument):
    cmd = icmd.format(argument)
    return cmd


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python lightningRestapiTest.py type hostname
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['headnode_ip', 'json', '--verbose'],
                              [str, str, None],
                              ['A host name or IP address for Headnode',
                               'json file',
                               'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        headnode = args.headnode_ip.split()[0]
        data = util.JSONparser(args.json)
        jump_headnode_cmd = generateCMD(CMD_HEADNODE, headnode)
        cmd = jump_headnode_cmd + generateCMD(CMD_TCP_LISTEN, LINKLOCAL_ADDR)
        try:
            f = subprocess.Popen(cmd,
                                 shell=True,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        except Exception:
            print("LightningRest-api tcp-listen [FAILED]")
        restapiTest(data=data, host=headnode, logger=logger)
        result, err = f.communicate()
    except Exception as e:
        print("Rest-api test [FAILED]")
        print("Error code returned: {}".format(e))
    sys.exit(1)
