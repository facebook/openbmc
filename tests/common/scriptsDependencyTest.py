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
import json


#TODO There are a few places this is used, needs to be moved to common util
def read_data(path):
    handle = open(path, "r")
    data = handle.readline().rstrip()
    handle.close()
    return data


def dependency_check(data):
    """
    Given a json of scripts:
    - if key is "read" assume it is shell read of file and it is accessible
    """
    for script, pdata in data['shell_scripts'].items():
        print("Checking script={}".format(script))
        for val in pdata["read"]:
            logger.debug("executing command: reading {}".format(val))
            try:
                rdata = read_data(val)
                print(rdata)
                if "NA" in rdata:
                    print("Dependency test for script {} [FAILED]".format(script))
            except Exception as e:
                print("Dependency test for scripts [FAILED]")
                print("Error code returned: " + str(e))
                sys.exit(1)
    print("Dependency test for scripts [PASSED]")
    sys.exit(0)

if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python scriptsDependencyTest.py.py dependency.json
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        data = {}
        args = util.Argparser(['json', '--verbose'], [str, None],
                              ['json file',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        data = util.JSONparser(args.json)
        dependency_check(data)
    except Exception as e:
        print("Dependency test for scripts [FAILED]")
        print("Error code returned: " + str(e))
    sys.exit(1)
