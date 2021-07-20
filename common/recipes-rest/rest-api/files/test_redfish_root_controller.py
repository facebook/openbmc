import asyncio
import unittest

import aiohttp.web
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler


class TestRootService(AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        asyncio.set_event_loop(asyncio.new_event_loop())

        self.patches = [
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_platform_name",
                return_value="FBY2",
            ),
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_uuid",
                new_callable=unittest.mock.MagicMock,
                return_value="bd7e0200-8227-3a1c-30c0-286261016903",
            ),
        ]

        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    @unittest_run_loop
    async def test_get_redfish(self):
        expected_resp = {"v1": "/redfish/v1"}
        req = await self.client.request("GET", "/redfish")
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_service_root(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#ServiceRoot.ServiceRoot",
            "@odata.id": "/redfish/v1/",
            "@odata.type": "#ServiceRoot.v1_3_0.ServiceRoot",
            "Id": "RootService",
            "Name": "Root Service",
            "Product": "FBY2",
            "RedfishVersion": "1.0.0",
            "UUID": "bd7e0200-8227-3a1c-30c0-286261016903",
            "Chassis": {"@odata.id": "/redfish/v1/Chassis"},
            "Managers": {"@odata.id": "/redfish/v1/Managers"},
            "SessionService": {"@odata.id": "/redfish/v1/SessionService"},
            "AccountService": {"@odata.id": "/redfish/v1/AccountService"},
        }
        req = await self.client.request("GET", "/redfish/v1")
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.setup_redfish_common_routes(webapp)
        return webapp
