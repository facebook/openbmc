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
from unittest import TestCase

try:
    import obmc_mmc
except ModuleNotFoundError:
    # Suppress if module not found to support test case discovery without
    # BMC dependencies (will test if module is importable at setUp())
    pass


class LibObmcMmcTest(TestCase):
    def setUp(self):
        super().setUp()

        # Ensure actual library is importable
        import obmc_mmc  # noqa: F401

    def test_list_devices(self):
        self.assertEqual(obmc_mmc.list_devices(), ["/dev/mmcblk0"])

    def test_mmc_dev_open_close(self):
        for path in obmc_mmc.list_devices():
            # Ensure we're able to open all /dev/mmcblk[0-9] devices
            mmc = obmc_mmc.mmc_dev_open(path)

            # Ensure closing them causes no error
            obmc_mmc.mmc_dev_close(mmc)

    def test_mmc_dev_open_non_existent(self):
        with self.assertRaises(obmc_mmc.LibObmcMmcException) as cm:
            obmc_mmc.mmc_dev_open("/non/existent")

        self.assertEqual(
            str(cm.exception), "Provided mmc device path '/non/existent' does not exist"
        )

    def test_mmc_cid_read(self):
        for path in obmc_mmc.list_devices():
            with obmc_mmc.mmc_dev(path) as mmc:
                # Ensure we're able to read the cid
                cid = obmc_mmc.mmc_cid_read(mmc)

                # Sanity test: ensure the manufacturer ID was set
                self.assertEqual(type(cid), obmc_mmc.mmc_cid_t)
                self.assertNotEqual(cid.mid, 0)

    def test_mmc_extcsd_read(self):
        for path in obmc_mmc.list_devices():
            with obmc_mmc.mmc_dev(path) as mmc:
                # Ensure we're able to read the extcsd
                extcsd = obmc_mmc.mmc_extcsd_read(mmc)

                self.assertEqual(type(extcsd), obmc_mmc.mmc_extcsd_t)

    def test_extcsd_life_time_estimate(self):
        for path in obmc_mmc.list_devices():
            with obmc_mmc.mmc_dev(path) as mmc:
                extcsd = obmc_mmc.mmc_extcsd_read(mmc)

                mmc_life = obmc_mmc.extcsd_life_time_estimate(extcsd)

                self.assertEqual(type(mmc_life), obmc_mmc.ExtCsdLifeTimeEstimate)

                self.assertEqual(type(mmc_life.EXT_CSD_PRE_EOL_INFO), int)
                self.assertEqual(type(mmc_life.EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A), int)
                self.assertEqual(type(mmc_life.EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B), int)
