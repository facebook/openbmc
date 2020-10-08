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
import unittest

from utils.cit_logger import Logger


PATH = "/tmp/sup_weutil.txt"


class SupeepromTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_sup_eeprom_file_presence(self):
        """
        Check if the sup eeprom file is present.
        The eeprom content is validated in test_eeprom.py
        """
        self.assertTrue(
            os.path.exists(PATH), "sup weutil path {} doesn't exist".format(PATH)
        )
