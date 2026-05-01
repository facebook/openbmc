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
import functools
import unittest

from common.base_rest_endpoint_test import BaseRestEndpointTest
from tests.fby35.test_data.restendpoints.restendpoints import REST_END_POINTS
from utils.test_utils import qemu_check


class RestEndpointTest(BaseRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    def verify_endpoint_attributes(self, endpointname, attributes):
        """
        re-write the parent function `verify_endpoint_attributes`
        for qemu environment. If CIT ran inside QEMU, than we only check for /api attribute.
        We won't continue to check resources since that will causing crash thus test
        will fail.
        """
        self.assertNotEqual(
            attributes, None, "{} endpoint attributes not set".format(endpointname)
        )
        info = self.get_from_endpoint(endpointname)
        for attrib in attributes:
            with self.subTest(attrib=attrib):
                self.assertIn(
                    attrib,
                    info,
                    msg="endpoint {} missing attrib {}".format(endpointname, attrib),
                )

    @classmethod
    def _gen_parameterised_tests(cls):
        # _test_usage_string_template -> test_usage_string_{cmd}
        if qemu_check():
            verify_method = cls.verify_endpoint_attributes
        else:
            verify_method = super().verify_endpoint_attributes
        for endpoint, resources in REST_END_POINTS.items():
            setattr(
                cls,
                "test_restendpoint_{}".format(endpoint),
                functools.partialmethod(
                    unittest.skipIf(
                        qemu_check() and endpoint != "/api", "test env is QEMU, skipped"
                    )(verify_method),
                    endpoint,
                    resources,
                ),
            )


RestEndpointTest._gen_parameterised_tests()
