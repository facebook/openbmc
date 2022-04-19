import aiohttp.web
import unittest
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler


class TestAccountService(AioHTTPTestCase):
    def setUp(self):
        super().setUp()

    @unittest_run_loop
    async def test_get_account_service(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#AccountService.AccountService",
            "@odata.id": "/redfish/v1/AccountService",
            "@odata.type": "#AccountService.v1_0_0.AccountService",
            "Id": "AccountService",
            "Name": "Account Service",
            "Description": "Account Service",
            "Accounts": {"@odata.id": "/redfish/v1/AccountService/Accounts"},
            "Roles": {"@odata.id": "/redfish/v1/AccountService/Roles"},
        }
        req = await self.client.request("GET", "/redfish/v1/AccountService")
        resp = await req.json()
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    async def get_application(self):
        with unittest.mock.patch(
            "aggregate_sensor.aggregate_sensor_init",
            create=True,
            return_value=None,
        ):
            webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
            from redfish_common_routes import Redfish

            redfish = Redfish()
            redfish.setup_redfish_common_routes(webapp)
            return webapp
