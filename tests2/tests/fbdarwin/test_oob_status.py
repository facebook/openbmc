#!/usr/bin/env python3
#
# Copyright 2026-present Facebook. All Rights Reserved.
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

import unittest

from common.base_oob_status_test import BaseOobStatusTest


class OobStatusTest(BaseOobStatusTest, unittest.TestCase):
    def setCases(self):
        self.cases = {
            "": 0,
            "0": 0,
            "1": 0,
            "IMP": 0,
            "1 2": 0,
            "2 1 0": 0,
            "IMP 0": 0,
            "0 1 2 IMP": 0,
            "3": 1,
            "0 1 3": 1,
            "0 1 2 3 IMP": 1,
        }
        self.invalid_text = "Invalid port 3"

    def setStatusDict(self):
        self.status_dict = {
            "link_status": {
                "0": "Link Up",
                "1": "Link Up",
                "2": "Link Down",
                "IMP": "Link Up",
            },
            "link_speed": {
                "0": "1000 Mb/s",
                "1": "1000 Mb/s",
                "2": "1000 Mb/s",
                "IMP": "1000 Mb/s",
            },
        }
