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
import sys
import unitTestUtil
import logging
import subprocess


def componentPresence():
    """
    Tests that the function /usr/local/bin/presence_util.sh is
    outputting propper data.
    """
    logger.debug("Executing command: /usr/local/bin/presence_util.sh")
    f = subprocess.Popen('sh /usr/local/bin/presence_util.sh',
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    info = info.decode('utf-8')
    if len(err) > 0:
        raise Exception(err + " [FAILED]")
    info = info.split('\n')

    # check that all component readings are 1
    success = True
    logger.debug("Checking component presence output")
    for line in info:
        if ':' in line:
            data = line.split(':')
            if 'peer_cmm' not in data[0] and '1' not in data[1]:
                success = False

    if success:
        print("component presence [PASSED]")
        sys.exit(0)
    else:
        print("component presence [FAILED]")
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python cmmComponentPresenceTest.py
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['--verbose'], [None],
                              ['output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        componentPresence()
    except Exception as e:
        print("Component presence Test [FAILED]")
        print("Error: " + str(e))
        sys.exit(1)
