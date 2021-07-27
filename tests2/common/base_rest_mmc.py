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
import urllib.request
from unittest import TestCase


class RestMMCTest(TestCase):
    """
    Validates fields in URL's .Information["mmc-info"] field in mmc-enabled
    devices
    """

    URL = "/api/sys/bmc"

    def test_mmc_info(self):
        req = urllib.request.Request("http://localhost:8080" + self.URL)
        resp = urllib.request.urlopen(req)
        resp_json = json.loads(resp.read().decode("utf-8"))

        mmc_info = resp_json["Information"]["mmc-info"]

        # XXX as of H2 2021 we support a single mmc device (at most)
        self.assertEqual(set(mmc_info.keys()), {"/dev/mmcblk0"})
        mmcblk0_info = resp_json["Information"]["mmc-info"]["/dev/mmcblk0"]

        self.assertGreater(mmcblk0_info["CID_MID"], 0)
        self.assertGreaterEqual(mmcblk0_info["CID_PRV_MAJOR"], 0)
        self.assertGreaterEqual(mmcblk0_info["CID_PRV_MINOR"], 0)
        self.assertGreaterEqual(mmcblk0_info["EXT_CSD_PRE_EOL_INFO"], 0)
        self.assertGreaterEqual(mmcblk0_info["EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A"], 0)
        self.assertGreaterEqual(mmcblk0_info["EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B"], 0)

        # CID_PNM must be a non-empty string
        self.assertRegex(mmcblk0_info["CID_PNM"], r".+")
