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
from tests.fbttn.test_data.restendpoints.restendpoints import REST_END_POINTS


class RestEndpointTest(BaseRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    @classmethod
    def _gen_parameterised_tests(cls):
        # _test_usage_string_template -> test_usage_string_{cmd}
        for endpoint, resources in REST_END_POINTS.items():
            setattr(
                cls,
                "test_restendpoint_{}".format(endpoint),
                functools.partialmethod(
                    cls.verify_endpoint_attributes,
                    endpoint,
                    resources,
                ),
            )


RestEndpointTest._gen_parameterised_tests()
