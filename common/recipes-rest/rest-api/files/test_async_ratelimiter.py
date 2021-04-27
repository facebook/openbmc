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
import unittest

import async_ratelimiter
import common_middlewares
from aiohttp import test_utils, web


def dummy_handler(request):
    return web.Response(
        body=json.dumps({"status": "ok"}), content_type="application/json"
    )


class TestRatelimiterDoesNotTriggerWhenLimitIs0(test_utils.AioHTTPTestCase):
    async def __assert_normal_request(self, meth="GET", useragent=None):
        headers = {"User-Agent": "test-agent"}
        if useragent:
            headers = {"User-Agent": useragent}
        request = await self.client.request(meth, "/test", headers=headers)
        self.assertEqual(request.status, 200)
        resp = await request.json()
        self.assertEqual({"status": "ok"}, resp)

    async def get_application(self):
        webapp = web.Application(middlewares=[common_middlewares.ratelimiter])
        webapp["ratelimiter"] = async_ratelimiter.AsyncRateLimiter(60, 0)
        webapp.router.add_get("/test", dummy_handler)
        return webapp

    @test_utils.unittest_run_loop
    async def test_ratelimiter_does_not_trigger(self):
        for _ in range(0, 10):
            await self.__assert_normal_request()


class TestRatelimiterWorks(test_utils.AioHTTPTestCase):
    async def __assert_normal_request(self, method="GET", useragent=None):
        headers = {"User-Agent": "test-agent"}
        if useragent:
            headers = {"User-Agent": useragent}
        request = await self.client.request(method, "/test", headers=headers)
        self.assertEqual(request.status, 200)
        resp = await request.json()
        self.assertEqual({"status": "ok"}, resp)

    async def get_application(self):
        webapp = web.Application(middlewares=[common_middlewares.ratelimiter])
        webapp["ratelimiter"] = async_ratelimiter.AsyncRateLimiter(20, 2)
        webapp.router.add_get("/test", dummy_handler)
        webapp.router.add_post("/test", dummy_handler)
        return webapp

    @test_utils.unittest_run_loop
    async def test_ratelimiter_triggers_when_limit_exceeded(self):
        for _ in range(0, 2):
            await self.__assert_normal_request()
        headers = {"User-Agent": "test-agent"}
        request = await self.client.request("GET", "/test", headers=headers)
        self.assertEqual(request.status, 429)

    @test_utils.unittest_run_loop
    async def test_ratelimiter_allows_other_useragents_but_blocks_blocked_ones(self):
        for _ in range(0, 2):
            await self.__assert_normal_request()
        headers = {"User-Agent": "test-agent"}
        request = await self.client.request("GET", "/test", headers=headers)
        self.assertEqual(request.status, 429)
        self.__assert_normal_request("GET", "agent2")

    @test_utils.unittest_run_loop
    async def test_ratelimiter_allows_other_methods_for_same_agent(self):
        for _ in range(0, 2):
            await self.__assert_normal_request()
        await self.__assert_normal_request("POST")
