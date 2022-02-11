#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
from utils.shell_util import run_cmd


class BaseMBootTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.check_list = set()
        self.set_check_list()

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    @abstractmethod
    def set_check_list(self):
        pass

    def test_mboot(self):
        """
        Test all measures
        """
        checked_list = set()
        cmd = ["/usr/bin/mboot-check", "-jt"]
        measures = json.loads(run_cmd(cmd))
        for m in measures:
            self.assertNotEqual(m["measure"], "NA")
            if m["component"] in self.check_list:
                self.assertEqual(m["expect"], m["measure"])
                checked_list.add(m["component"])
        self.assertEqual(checked_list, self.check_list)
