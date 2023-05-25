#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

from common.base_image_layout_test import BaseImageLayoutTest
from utils.test_utils import qemu_check

@unittest.skip("Skip until test is updated")
class ImageLayoutTest(BaseImageLayoutTest):
    IMAGE_LAYOUT = {
        "spi0.0": {
            "meta_name": "metaro",
            "mapping": {
                # dmesg name: mtd name
                "rom": "spl",
                "recovery": "rec-u-boot",
                "metaro": "image-meta",
                "fitro": "os-fit",
            },
        },
        "spi0.1": {
            "meta_name": "meta",
            "mapping": {
                "romx": "spl",
                "meta": "image-meta",
                "fit": "os-fit",
                "env": "u-boot-env",
            },
        },
    }

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_spi_1_mtd(self):
        """
        Test actual image layout on spi0.1 matches image meta data
        """
        super().test_spi_1_mtd()
