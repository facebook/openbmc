import asyncio

import aiohttp.web
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler
from redfish_computer_system import get_compute_system_names
from test_redfish_computer_system_patches import patches


class TestComputerSystems(AioHTTPTestCase):
    async def setUpAsync(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        self.patches = patches()
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)
        await super().setUpAsync()

    @unittest_run_loop
    async def test_get_compute_system_names(self):
        expected_names = ["server1", "server2", "server3", "server4"]
        names = get_compute_system_names()
        self.assertEqual(names, expected_names)

    @unittest_run_loop
    async def test_get_collection_descriptor(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#ComputerSystemCollection.ComputerSystemCollection",  # noqa: B950
            "@odata.id": "/redfish/v1/Systems",
            "@odata.type": "#ComputerSystemCollection.ComputerSystemCollection",
            "Name": "Computer System Collection",
            "Members@odata.count": 4,
            "Members": [
                {"@odata.id": "/redfish/v1/Systems/server1", "Name": "server1"},
                {"@odata.id": "/redfish/v1/Systems/server2", "Name": "server2"},
                {"@odata.id": "/redfish/v1/Systems/server3", "Name": "server3"},
                {"@odata.id": "/redfish/v1/Systems/server4", "Name": "server4"},
            ],
        }
        req = await self.client.request("GET", "/redfish/v1/Systems")
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_get_system_descriptor(self):
        expected_resp = {
            "@odata.id": "/redfish/v1/Systems/server1",
            "@odata.type": "#ComputerSystem.v1_16_1.ComputerSystem",
            "Id": "server1",
            "Name": "server1",
            "Actions": {
                "#ComputerSystem.Reset": {
                    "ResetType@Redfish.AllowableValues": [
                        "On",
                        "ForceOff",
                        "GracefulShutdown",
                        "GracefulRestart",
                        "ForceRestart",
                        "ForceOn",
                        "PushPowerButton",
                    ],
                    "target": "/redfish/v1/Systems/server1/Actions/ComputerSystem.Reset",
                }
            },
            "Bios": {
                "@odata.id": "/redfish/v1/Systems/server1/Bios",
            },
            "LogServices": {"@odata.id": "/redfish/v1/Systems/server1/LogServices"},
        }
        req = await self.client.request("GET", "/redfish/v1/Systems/server1")
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

        req = await self.client.request("GET", "/redfish/v1/Systems/server5")
        self.assertEqual(req.status, 404)

    @unittest_run_loop
    async def test_get_bios_descriptor(self):
        expected_resp = {
            "@odata.id": "/redfish/v1/Systems/server1/Bios",
            "@odata.type": "#Bios.v1_1_0.Bios",
            "Id": "server1 BIOS",
            "Name": "server1 BIOS",
            "Links": {
                "ActiveSoftwareImage": {
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareInventory",
                    "@odata.type": "#SoftwareInventory.v1_0_0.SoftwareInventory",
                    "Id": "server1/Bios",
                    "Name": "server1 BIOS Firmware",
                },
            },
        }
        req = await self.client.request("GET", "/redfish/v1/Systems/server1/Bios")
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

        req = await self.client.request("GET", "/redfish/v1/Systems/server5/Bios")
        self.assertEqual(req.status, 404)

    @unittest_run_loop
    async def test_get_bios_firmware_inventory(self):
        expected_resp = {
            "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareInventory",
            "@odata.type": "#SoftwareInventory.v1_0_0.SoftwareInventory",
            "Id": "server1/Bios",
            "Name": "server1 BIOS Firmware",
            "Oem": {
                "Dumps": {
                    "@odata.context": "/redfish/v1/$metadata#FirmwareDumps.FirmwareDumps",
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps",
                    "@odata.type": "#FirmwareDumps.v1_0_0.FirmwareDumps",
                    "Id": "server1/Bios Dumps",
                    "Name": "server1/Bios Dumps",
                }
            },
        }
        req = await self.client.request(
            "GET", "/redfish/v1/Systems/server1/Bios/FirmwareInventory"
        )
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

        req = await self.client.request(
            "GET", "/redfish/v1/Systems/server5/Bios/FirmwareInventory"
        )
        self.assertEqual(req.status, 404)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])

        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.setup_redfish_common_routes(webapp)
        return webapp
