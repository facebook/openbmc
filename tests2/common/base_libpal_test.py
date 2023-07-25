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
import math
from unittest import TestCase

try:
    import pal
    import sdr
except ModuleNotFoundError:
    # Suppress if module not found to support test case discovery without
    # BMC dependencies (will test if pal is importable at LibPalTest.setUp())
    pass


class LibPalTest(TestCase):
    # Some platforms don't return FRU info through libpal, track them here
    PLATFORMS_WITHOUT_FRU_INFO = [
        "yamp",
        "wedge",
        "wedge100",
    ]

    # The platform name as expected by pal_get_platform_name().
    PLATFORM_NAME = None

    def setUp(self):
        super().setUp()

        # Ensure actual pal library is importable (as we suppressed with
        # ModuleNotFoundError earlier)
        import pal  # noqa: F401
        import sdr  # noqa: F401

        # Validate if PLATFORM_NAME was set in subclasses
        self.assertRegex(
            getattr(self, "PLATFORM_NAME", None) or "",
            r"^[a-z0-9]+$",
            "Test error: Expected {cls}.PLATFORM_NAME to be a valid platform name, got {PLATFORM_NAME}".format(  # noqa: B950
                cls=repr(self.__class__),
                PLATFORM_NAME=repr(getattr(self, "PLATFORM_NAME", None)),
            ),
        )

    def test_pal_get_platform_name(self):
        plat_name = pal.pal_get_platform_name()

        self.assertEqual(plat_name, self.PLATFORM_NAME)

    def test_pal_get_fru_list(self):
        fru_list = pal.pal_get_fru_list()

        self.assertEqual(type(fru_list), tuple)

        if self._plat_has_no_fru_info():
            self.assertEqual(
                fru_list,
                (),
                "expected empty FRU list from platform without libpal FRUs",
            )
        else:
            self.assertGreater(len(fru_list), 0, "pal_get_fru_list() returned no FRUs")

        # Assert all FRU names are non-empty strings
        for fru_name in fru_list:
            self.assertRegex(fru_name, r"^[^ ]+$")

        # Assert fake catch-all FRU is not in the list
        self.assertNotIn("all", fru_list)

    def test_pal_get_fru_id(self):
        fru_list = pal.pal_get_fru_list()

        seen = {}  # track duplicates

        for fru_name in fru_list:
            # FRU names listed in pal_get_fru_list() MUST have a corresponding id
            fru_id = pal.pal_get_fru_id(fru_name)

            self.assertEqual(type(fru_id), int)
            self.assertNotIn(
                fru_id,
                seen,
                "duplicate fru_id ({fru_id}) for {fru_name} and {seen_fru_name}".format(
                    fru_id=fru_id,
                    fru_name=repr(fru_name),
                    seen_fru_name=repr(seen.get(fru_id)),
                ),
            )
            seen[fru_id] = fru_name

    def test_pal_get_fru_id_invalid(self):
        with self.assertRaises(ValueError) as cm:
            pal.pal_get_fru_id("alskdjaksldjklsa")

        self.assertEqual(str(cm.exception), "Invalid FRU name: 'alskdjaksldjklsa'")

    def test_pal_fru_name_map(self):
        fru_list = pal.pal_get_fru_list()

        fru_map = pal.pal_fru_name_map()

        self.assertEqual(
            fru_map, {fru_name: pal.pal_get_fru_id(fru_name) for fru_name in fru_list}
        )

    def test_pal_get_fru_sensor_list(self):
        if self._plat_has_no_fru_info():
            self.skipTest(
                "Platform has no FRU info, not testing pal_get_fru_sensor_list()"
            )

        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            sensor_list = pal.pal_get_fru_sensor_list(fru_id)

            self.assertEqual(type(sensor_list), tuple)
            self.assertTrue(all(type(x) == int for x in sensor_list))

    def test_pal_get_sensor_name(self):
        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            if not pal.pal_is_fru_prsnt(fru_id):
                continue
            for snr_num in pal.pal_get_fru_sensor_list(fru_id):
                sensor_name = sdr.sdr_get_sensor_name(fru_id, snr_num)

                self.assertRegex(sensor_name, r"^[^ ]+$")

    def test_pal_is_fru_prsnt(self):
        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            is_fru_prsnt = pal.pal_is_fru_prsnt(fru_id)
            self.assertTrue(type(is_fru_prsnt), True)

    def test_sensor_raw_read(self, skip_sensors=[]):
        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            if not pal.pal_is_fru_prsnt(fru_id):
                continue

            for snr_num in pal.pal_get_fru_sensor_list(fru_id):
                if [fru_id, snr_num] in skip_sensors:
                    continue

                with self.subTest("fru_id {} snr_num {}".format(fru_id, snr_num)):
                    val = pal.sensor_raw_read(fru_id, snr_num)

                    self.assertEqual(type(val), float)

                    # Ensure value is a real number (i.e. not NaN or INF)
                    self.assertTrue(math.isfinite(val))

    def test_sensor_read(self, skip_sensors=[]):
        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            # Only test the behaviour of present FRUs (see
            # test_sensor_read_fru_not_present for what should happen if the FRU
            # is not present)
            if not pal.pal_is_fru_prsnt(fru_id):
                continue

            for snr_num in pal.pal_get_fru_sensor_list(fru_id):
                if [fru_id, snr_num] in skip_sensors:
                    continue

                with self.subTest("fru_id {} snr_num {}".format(fru_id, snr_num)):
                    val = pal.sensor_read(fru_id, snr_num)

                    self.assertEqual(type(val), float)

                    # Ensure value is a real number (i.e. not NaN or INF)
                    self.assertTrue(math.isfinite(val))

    def test_sensor_read_fru_not_present(self):
        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            if pal.pal_is_fru_prsnt(fru_id):
                continue

            for snr_num in pal.pal_get_fru_sensor_list(fru_id):
                # MUST raise LibPalError if fru is not present
                with self.assertRaises(pal.LibPalError):
                    pal.sensor_read(fru_id, snr_num)

    def _plat_has_no_fru_info(self):
        plat_name = pal.pal_get_platform_name()
        return plat_name in self.PLATFORMS_WITHOUT_FRU_INFO
