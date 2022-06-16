import asyncio
import unittest

import aiohttp
import redfish_fwinfo as sut
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler

MOCK_FWUTIL_RESP = """ALTBMC Version: NA
ALTBMC Version After Activated: NA
BMC Version: wedge100-v2022.03.1
BMC Version After Activated: wedge100-v2022.03.1
Fan Speed Controller Version: error_returned
TPM Version: 133.33"""

MOCK_FWUTIL_RESULT = (0, MOCK_FWUTIL_RESP, None)

CPLD_REV_RESP = "6.101"
CPLD_REV_RESULT = (0, CPLD_REV_RESP, None)

CPLD_VER_RESP = """SMB_SYSCPLD: 4.4
SMB_PWRCPLD: 4.1
SCMCPLD: 4.1
FCMCPLD: 4.2

"""
CPLD_VER_RESULT = (0, CPLD_VER_RESP, None)

FPGA_VER_RESP = """------SCM-FPGA------
SCM_FPGA: 2.14
------FAN-FPGA------
FAN_FPGA: 1.11
------SMB-FPGA------
SMB_FPGA: 3.8
------SMB-CPLD------
SMB_CPLD: 5.1
------PIM-FPGA------
PIM 2: 7.4
PIM 3: 7.4
PIM 4: 7.4
PIM 5: 7.4
PIM 6: 7.4
PIM 7: 7.4
PIM 8: 7.4
PIM 9: 7.4

"""
FPGA_VER_RESULT = (0, FPGA_VER_RESP, None)


class TestRedfishFWInfo(AioHTTPTestCase):
    def setUp(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        futs = [asyncio.Future(), asyncio.Future(), asyncio.Future(), asyncio.Future()]
        futs[0].set_result(MOCK_FWUTIL_RESULT)
        futs[1].set_result(CPLD_REV_RESULT)
        futs[2].set_result(CPLD_VER_RESULT)
        futs[3].set_result(FPGA_VER_RESULT)
        self.patches = [
            unittest.mock.patch(
                "common_utils.async_exec",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                side_effect=futs,
            ),
            unittest.mock.patch("os.path.exists", return_value=True),
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)
        # redfish_powercycle._verify_command_status.return_value.set_result(True)
        super().setUp()

    @unittest_run_loop
    async def test_updateservice_root(self):
        expected_resp = {
            "@odata.type": "#UpdateService.v1_8_0.UpdateService",
            "@odata.id": "/redfish/v1/UpdateService",
            "Id": "UpdateService",
            "Name": "Update service",
            "Status": {"State": "Enabled", "Health": "OK", "HealthRollup": "OK"},
            "ServiceEnabled": False,
            "HttpPushUri": "/FWUpdate",
            "HttpPushUriOptions": {
                "HttpPushUriApplyTime": {
                    "ApplyTime": "Immediate",
                    "ApplyTime@Redfish.AllowableValues": [
                        "Immediate",
                        "OnReset",
                        "AtMaintenanceWindowStart",
                        "InMaintenanceWindowOnReset",
                    ],
                    "MaintenanceWindowStartTime": "2018-12-01T03:00:00+06:00",
                    "MaintenanceWindowDurationInSeconds": 600,
                }
            },
            "HttpPushUriOptionsBusy": False,
            "FirmwareInventory": {
                "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory"
            },
            "Actions": {},
            "Oem": {},
        }
        req = await self.client.request("GET", "/redfish/v1/UpdateService")
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_firmwareinventory_root(self):
        expected_resp = {
            "@odata.type": "#SoftwareInventoryCollection.SoftwareInventoryCollection",
            "Name": "Firmware Collection",
            "Members@odata.count": 20,
            "Members": [
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/BMC"},
                {
                    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/FAN_SPEED_CONTROLLER"
                },
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/TPM"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/CPLD"},
                {
                    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/SMB_SYSCPLD"
                },
                {
                    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/SMB_PWRCPLD"
                },
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/SCMCPLD"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/FCMCPLD"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/SCM_FPGA"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/FAN_FPGA"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/SMB_FPGA"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/SMB_CPLD"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_2"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_3"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_4"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_5"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_6"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_7"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_8"},
                {"@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/PIM_9"},
            ],
            "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory",
        }
        req = await self.client.request(
            "GET", "/redfish/v1/UpdateService/FirmwareInventory"
        )
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_firmwareinventory_root_child(self):
        expected_resp = {
            "@odata.type": "#SoftwareInventory.v1_2_3.SoftwareInventory",
            "Id": "BMC",
            "Name": "BMC Firmware",
            "Status": {"State": "Enabled", "Health": "OK"},
            "Manufacturer": "Facebook",
            "Version": "wedge100-v2022.03.1",
            "SoftwareId": "wedge100-v2022.03.1",
            "Actions": {"Oem": {}},
            "Oem": {},
            "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/BMC",
        }
        req = await self.client.request(
            "GET", "/redfish/v1/UpdateService/FirmwareInventory/BMC"
        )
        self.assertEqual(req.status, 200)
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_firmwareinventory_root_child_nonexistent(self):
        req = await self.client.request(
            "GET", "/redfish/v1/UpdateService/FirmwareInventory/DERP"
        )
        self.assertEqual(req.status, 404)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])

        webapp.router.add_get(
            "/redfish/v1/UpdateService",
            sut.get_update_service_root,
        )
        webapp.router.add_get(
            "/redfish/v1/UpdateService/FirmwareInventory",
            sut.get_firmware_inventory_root,
        )
        webapp.router.add_get(
            "/redfish/v1/UpdateService/FirmwareInventory/{fw_name}",
            sut.get_firmware_inventory_child,
        )
        return webapp
