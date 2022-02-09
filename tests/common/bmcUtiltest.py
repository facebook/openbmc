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
import subprocess
import unitTestUtil
import logging


def bmc_util_check(data):
    """
    Given a json of shell scripts, check that all of them run and have
    exited properly. Take care to have only non-disruptive scripts here
    """
    for script in data:
        print("executing command: {}".format(script))
        f = subprocess.Popen(script,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        failed = []
        if f.returncode is not None:
            failed += [str(script)]
    if len(failed) == 0:
        print("BMC util Test: All scripts ran successfully [PASSED]")
        sys.exit(0)
    else:
        print("BMC util Test Failed scripts: " + str(failed) + "[FAILED]")
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python bmcUtilTest.py bmcUtil.json
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        data = {}
        args = util.Argparser(['json', '--verbose'],
                              [str, None],
                              ['json file',
                              'output all steps from test with mode options:\
                              DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        data = util.JSONparser(args.json)
        bmc_util_check(data)
    except Exception as e:
        print("BMC util test [FAILED]")
        print("Error code returned: " + str(e))
    sys.exit(1)
