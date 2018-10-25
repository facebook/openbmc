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
import subprocess
import sys
import unitTestUtil
import logging

PING_CMDS = {4: "ping -c 1 -q -w 1 {}", 6: "ping6 -c 1 -q -w 1 {}"}

def pingTest(ping, version, logger, cmd):
    """
    Ping a host from outside the platform
    """
    logger.debug("executing ipv{} ping command".format(version))
    cmd = cmd.format(ping)
    f = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    rst, err = f.communicate()
    if "1 received, 0% packet loss".encode('utf-8') not in rst:
        raise Exception(err)
    logger.debug("ping{} command successfully executed".format(version))
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
        version = args.version
        pingcmd = PING_CMDS[version]
        version = str(version)
        pingTest(ping, version, logger, pingcmd)
    except Exception as e:
        print("Ping Test [FAILED]")
        print("Error: " + str(e))
        sys.exit(1)
