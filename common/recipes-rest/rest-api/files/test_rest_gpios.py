#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

import sys
import types
import unittest

import aiohttp.web
import rest_fruid
from aiohttp.test_utils import AioHTTPTestCase

rest_gpios = None  # Module will be imported in setUp


class TestRestGpios(AioHTTPTestCase):
    def setUp(self):
        global rest_gpios
        super().setUp()

        # XXX We should have a better, more realistic mock for libgpio
        # Working around for now with a dumb mock
        sys.modules["rest_helper"] = types.ModuleType("rest_helper")
        sys.modules["rest_helper"].read_gpio_by_shadow = unittest.mock.Mock(
            wraps=lambda x: -1
        )

        self.patches = [
            unittest.mock.patch(
                "rest_fruid.get_fruid",
                autospec=True,
                return_value={"Information": {"Product Name": "Wedge-DC-F"}},
            )
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

        # Has to be imported here to avoid `ImportError: No module named 'libgpio'`
        # (see mocked rest_helper above)
        import rest_gpios

        # Clear _check_wedge cache before every test
        rest_gpios._check_wedge.cache_clear()

    def cleanUp(self):
        # Disable rest_helper mock
        sys.modules.pop("rest_helper")

    def test_check_wedge_ac(self):
        rest_fruid.get_fruid.return_value = {
            "Information": {"Product Name": "Wedge-AC-F"}
        }
        is_wedge, is_wedge40 = rest_gpios._check_wedge()
        self.assertEqual(is_wedge, True)
        self.assertEqual(is_wedge40, True)

    def test_check_wedge_dc(self):
        rest_fruid.get_fruid.return_value = {
            "Information": {"Product Name": "Wedge-DC-F"}
        }
        is_wedge, is_wedge40 = rest_gpios._check_wedge()
        self.assertEqual(is_wedge, True)
        self.assertEqual(is_wedge40, True)

    def test_check_wedge_wedge100(self):
        rest_fruid.get_fruid.return_value = {
            "Information": {"Product Name": "WEDGE100S12V"}
        }
        is_wedge, is_wedge40 = rest_gpios._check_wedge()
        self.assertEqual(is_wedge, True)
        self.assertEqual(is_wedge40, False)

    def test_check_wedge_yamp100(self):
        rest_fruid.get_fruid.return_value = {"Information": {"Product Name": "YAMP"}}
        is_wedge, is_wedge40 = rest_gpios._check_wedge()
        self.assertEqual(is_wedge, False)
        self.assertEqual(is_wedge40, False)

    def test_check_wedge_memoised(self):
        self.assertEqual(rest_gpios._check_wedge(), (True, True))
        rest_fruid.get_fruid.assert_called_once_with()

        # rest_fruid.get_fruid shouldn't be called in subsequent calls
        rest_fruid.get_fruid.reset_mock()
        self.assertEqual(rest_gpios._check_wedge(), (True, True))
        self.assertEqual(rest_fruid.get_fruid.mock_calls, [])

    def test_get_gpios_wedge(self):
        with unittest.mock.patch(
            "rest_gpios.read_wedge_back_ports",
            autospec=True,
            return_value={"<mock_read_wedge_backports_output"},
        ):
            result = rest_gpios.get_gpios()
            self.assertEqual(
                result, {"back_ports": {"<mock_read_wedge_backports_output"}}
            )

    def test_get_gpios_not_wedge(self):
        rest_fruid.get_fruid.return_value = {"Information": {"Product Name": "YAMP"}}
        with unittest.mock.patch(
            "rest_gpios.read_wedge_back_ports",
            autospec=True,
            return_value={"<mock_read_wedge_backports_output"},
        ):
            result = rest_gpios.get_gpios()
            self.assertEqual(result, {})

    async def get_application(self):
        return aiohttp.web.Application()
