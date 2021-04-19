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

import asyncio
import json
import typing as t
import unittest

import common_auth
import common_middlewares
from acl_providers import common_acl_provider_base
from aiohttp import test_utils, web
from common_middlewares import auth_enforcer


def async_return(result):
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    f = asyncio.Future()
    f.set_result(result)
    return f


TEST_ACL_RULES = {
    "/authenticated": {"GET": ["test"]},
    "/youshallnotpass": {"GET": ["youshallnotpass"]},
    "/authorized": {"GET": ["test"]},
    "/post_with_rbac": {"POST": ["test"]},
}


def good_handler(request):
    return web.Response(
        body=json.dumps({"status": "ok"}), content_type="application/json"
    )


def identity_handler(request):
    return web.Response(
        body=json.dumps({"identity": request.get("identity")}),
        content_type="application/json",
    )


def add_identity_to_request(request):
    request["identity"] = "test_identity"


class TestAclProvider(common_acl_provider_base.AclProviderBase):
    def is_user_authorized(
        self, identity: common_auth.Identity, permissions: t.List[str]
    ) -> bool:
        return "test" in permissions


class TestAuthEnforcer(test_utils.AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        self.patches = [
            unittest.mock.patch(
                "common_auth._validate_cert_date", autospec=True, return_value=True
            ),
            unittest.mock.patch(
                "common_middlewares._is_request_from_localhost",
                autospec=True,
                return_value=False,
            ),
            unittest.mock.patch("acl_config.RULES", new=TEST_ACL_RULES),
            unittest.mock.patch(
                "common_auth.auth_required",
                autospec=True,
                side_effect=add_identity_to_request,
            ),
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    async def get_application(self):
        webapp = web.Application(middlewares=[auth_enforcer])
        webapp.router.add_get("/authenticated", identity_handler)
        webapp.router.add_get("/authorized", identity_handler)
        webapp.router.add_get("/youshallnotpass", identity_handler)
        webapp.router.add_get("/regular_endpoint", good_handler)
        webapp.router.add_post("/post_default_deny", good_handler)
        webapp.router.add_post("/post_with_rbac", good_handler)
        webapp["acl_provider"] = TestAclProvider()
        return webapp

    @test_utils.unittest_run_loop
    async def test_authentication_extracts_identity(self):
        request = await self.client.request("GET", "/authenticated")
        self.assertEqual(request.status, 200)
        resp = await request.json()
        self.assertEqual({"identity": "test_identity"}, resp)

    @test_utils.unittest_run_loop
    async def test_authorization_passes_with_matching_roles(self):
        request = await self.client.request("GET", "/authorized")
        self.assertEqual(request.status, 200)
        resp = await request.json()
        self.assertEqual({"identity": "test_identity"}, resp)

    @test_utils.unittest_run_loop
    async def test_authorization_fails_with_disjunct_roles(self):
        request = await self.client.request("GET", "/youshallnotpass")
        self.assertEqual(request.status, 403)

    @test_utils.unittest_run_loop
    async def test_authorization_override_if_localhost(self):
        common_middlewares._is_request_from_localhost.return_value = True
        request = await self.client.request("GET", "/youshallnotpass")
        self.assertEqual(request.status, 200)

    @test_utils.unittest_run_loop
    async def test_authorization_denies_post_without_rules(self):
        request = await self.client.request("POST", "/post_default_deny")
        self.assertEqual(request.status, 403)

    @test_utils.unittest_run_loop
    async def test_regular_endpoints_require_no_auth(self):
        request = await self.client.request("GET", "/regular_endpoint")
        self.assertEqual(request.status, 200)
        resp = await request.json()
        self.assertEqual({"status": "ok"}, resp)

    @test_utils.unittest_run_loop
    async def test_post_allows_with_proper_permissions(self):
        request = await self.client.request("POST", "/post_with_rbac")
        self.assertEqual(request.status, 200)
        resp = await request.json()
        self.assertEqual({"status": "ok"}, resp)
