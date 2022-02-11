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

import os
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseKvTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.kv_keys = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    @abstractmethod
    def set_kv_keys(self):
        pass

    def test_kv_path_exists(self):
        cmd = "/usr/bin/kv"
        self.assertTrue(
            os.path.exists(cmd),
            "kv path {} is not present".format(cmd),
        )

    def test_kv_get_key(self):
        self.set_kv_keys()
        self.assertNotEqual(self.kv_keys, None, "Expected set of kv keys not set")

        for key in self.kv_keys:
            with self.subTest(key=key):
                cmd = "/usr/bin/kv get {} persistent".format(key)
                run_shell_cmd(cmd)
