import asyncio
import unittest

import aiohttp.web
import redfish_chassis_helper
import test_mock_modules  # noqa: F401
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler


class TestChassisService(AioHTTPTestCase):
    def setUp(self):
        asyncio.set_event_loop(asyncio.new_event_loop())

        self.patches = [
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_num_slots",
                create=True,
                return_value=4,
            ),
            unittest.mock.patch(
                "pal.pal_get_platform_name",
                create=True,
                return_value="fby2",
            ),
            unittest.mock.patch(
                "pal.pal_fru_name_map",
                create=True,
                return_value={
                    "slot1": 1,
                    "slot2": 2,
                    "slot3": 3,
                    "slot4": 4,
                    "spb": 5,
                    "bmc": 6,
                },
            ),
            unittest.mock.patch(
                "pal.pal_is_fru_prsnt",
                create=True,
                return_value=True,
            ),
            unittest.mock.patch(
                "pal.LibPalError",
                create=True,
                new=type("LibPalError", (Exception,), {}),
            ),
            unittest.mock.patch(
                "redfish_chassis_helper.get_fru_info",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=asyncio.Future(),
            ),
            unittest.mock.patch(
                "redfish_chassis_helper.get_single_sled_frus",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=["bmc", "spb"],
            ),
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

        super().setUp()

    def get_fru_name(self, server_name: str) -> str:
        if server_name == "1":
            return "spb"  # default to assert single sled frus
        else:
            #  if not a single sled fru then return the correct fru_name for assertion
            return server_name.replace("server", "slot")

    @unittest_run_loop
    async def test_get_chassis(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",  # noqa: B950
            "@odata.id": "/redfish/v1/Chassis",
            "@odata.type": "#ChassisCollection.ChassisCollection",
            "Name": "Chassis Collection",
            "Members@odata.count": 5,
            "Members": [
                {"@odata.id": "/redfish/v1/Chassis/1"},
                {"@odata.id": "/redfish/v1/Chassis/server1"},
                {"@odata.id": "/redfish/v1/Chassis/server2"},
                {"@odata.id": "/redfish/v1/Chassis/server3"},
                {"@odata.id": "/redfish/v1/Chassis/server4"},
            ],
        }
        req = await self.client.request("GET", "/redfish/v1/Chassis")
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_chassis_members(self):
        "Testing chassis members for both single sled frus and multisled frus"
        for server_name in ["1", "server1", "server2", "server3", "server4"]:
            fru_name = self.get_fru_name(server_name)
            with self.subTest(server_name=server_name):
                redfish_chassis_helper.get_fru_info.return_value = asyncio.Future()
                redfish_chassis_helper.get_fru_info.return_value.set_result(
                    redfish_chassis_helper.FruInfo(
                        fru_name, "Wiwynn", "WTL19121DSMA1", "Yosemite V2 MP"
                    )
                )
                if server_name == "1":
                    expected_name = "Computer System Chassis"
                else:
                    expected_name = fru_name

                expected_resp = {
                    "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
                    "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
                    "@odata.type": "#Chassis.v1_15_0.Chassis",
                    "Id": "1",
                    "Name": expected_name,
                    "ChassisType": "RackMount",
                    "Manufacturer": "Wiwynn",
                    "Model": "Yosemite V2 MP",
                    "SerialNumber": "WTL19121DSMA1",
                    "PowerState": "On",
                    "Sensors": {
                        "@odata.id": "/redfish/v1/Chassis/{}/Sensors".format(
                            server_name
                        )
                    },
                    "Status": {"State": "Enabled", "Health": "OK"},
                    "Links": {"ManagedBy": [{"@odata.id": "/redfish/v1/Managers/1"}]},
                }
                req = await self.client.request(
                    "GET", "/redfish/v1/Chassis/{}".format(server_name)
                )
                resp = await req.json()
                self.maxDiff = None
                self.assertEqual(resp, expected_resp)
                self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_multislot_routes_return_notfound_on_singleslot(self):
        with unittest.mock.patch(
            "rest_pal_legacy.pal_get_num_slots",
            create=True,
            return_value=1,
        ):
            req = await self.client.request("GET", "/redfish/v1/Chassis/server4")
            self.assertEqual(req.status, 404)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.setup_redfish_common_routes(webapp)
        return webapp
