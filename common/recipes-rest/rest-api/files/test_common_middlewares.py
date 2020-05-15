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

import json

from aiohttp import test_utils, web
from common_middlewares import jsonerrorhandler


def bad_handler(request):
    raise ValueError("OOPS")
    return web.Response(body=json.dumps({"status": "ok"}))


def good_handler(request):
    return web.Response(
        body=json.dumps({"status": "ok"}), content_type="application/json"
    )


class TestErrorHandler(test_utils.AioHTTPTestCase):
    async def get_application(self):
        """
        Override the get_app method to return your application.
        """
        webapp = web.Application(middlewares=[jsonerrorhandler])
        webapp.router.add_get("/bad", bad_handler)
        webapp.router.add_get("/good", good_handler)
        return webapp

    @test_utils.unittest_run_loop
    async def test_internal_server_error_case(self):
        self.maxDiff = None
        r = repr(ValueError("OOPS"))
        request = await self.client.request("GET", "/bad")
        assert request.status == 500
        resp = await request.json()
        self.assertEqual(
            {"Information": {"reason": r}, "Actions": [], "Resources": []}, resp
        )

    @test_utils.unittest_run_loop
    async def test_proper_response(self):
        request = await self.client.request("GET", "/good")
        assert request.status == 200
        resp = await request.json()
        self.assertEqual({"status": "ok"}, resp)
