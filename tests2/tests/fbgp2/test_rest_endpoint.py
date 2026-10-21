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

from common.base_rest_endpoint_test import BaseRestEndpointTest


# Minimal REST sanity check, intentionally NOT skipped under QEMU. A rest.py
# that dies at import is respawned by runit forever and most other tests still
# pass; this fails directly instead of relying on side effects such as CPU
# utilization. setUpClass waits up to WAIT_UNTIL_REACHABLE_TIMEOUT for /api.
class RestEndpointTest(BaseRestEndpointTest, unittest.TestCase):
    API_ATTRIBUTES = ["Description", "version"]

    def test_restendpoint_api(self):
        info = self.get_from_endpoint("/api")
        for attrib in self.API_ATTRIBUTES:
            with self.subTest(attrib=attrib):
                self.assertIn(
                    attrib.lower(),
                    info.lower(),
                    msg="endpoint /api missing attrib {}".format(attrib),
                )
