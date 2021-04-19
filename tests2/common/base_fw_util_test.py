#!/usr/bin/env python3
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


import json
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseFwUtilTest(object):
    """[summary] Base class for fw_util test
    To provide common function
    """

    def verify_output(self, cmd, str_wanted, msg=None):
        Logger.info("executing cmd = {}".format(cmd))
        out = run_shell_cmd(cmd)
        self.assertIn(str_wanted, out, msg=msg)

    @abstractmethod
    def set_platform(self):
        pass

    def setUp(self):
        self.set_platform()
        with open(
            "/usr/local/bin/tests2/tests/{}/test_data/fwutil/fw_util_list.json".format(
                self.platform
            )
        ) as f:
            self.fw_util_info = json.load(f)


class CommonFwUtilTest(BaseFwUtilTest):
    """[summary] A common test class for all fw_util test"""

    pass
