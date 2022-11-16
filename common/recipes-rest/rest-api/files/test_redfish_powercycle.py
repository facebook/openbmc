import asyncio
import unittest

import aiohttp
import common_utils
import redfish_powercycle
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler

MOCK_RESP = "Mocking fru/server to turn on/off/reboot"
MOCK_ASYNC_EXEC = (0, MOCK_RESP, None)


class TestPowerCycleService(AioHTTPTestCase):
    async def setUpAsync(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        self.patches = [
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_num_slots",
                create=True,
                return_value=4,
            ),
            unittest.mock.patch(
                "redfish_powercycle.is_power_util_available",
                return_value=True,
            ),
            unittest.mock.patch(
                "redfish_powercycle.is_wedge_power_available",
                return_value=True,
            ),
            unittest.mock.patch(
                "common_utils.async_exec",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=asyncio.Future(),
            ),
            unittest.mock.patch(
                "os.spawnvpe",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value="",
            ),
        ]

        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)
        common_utils.async_exec.return_value.set_result(MOCK_ASYNC_EXEC)
        # redfish_powercycle._verify_command_status.return_value.set_result(True)
        await super().setUpAsync()

    @unittest_run_loop
    async def test_systems_powercycle_compute(self):
        with unittest.mock.patch(
            "redfish_powercycle.is_power_util_available"
        ) as is_avail:
            is_avail.return_value = True
            req = await self.client.request(
                "POST",
                "/redfish/v1/Systems/server1/Actions/ComputerSystem.Reset",
                json={"ResetType": "On"},
            )
            self.assertEqual(
                await req.json(),
                {
                    "Response": "Mocking fru/server to turn on/off/reboot",
                    "status": "OK",
                },
            )
            self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_systems_powercycle_fboss(self):
        with unittest.mock.patch(
            "redfish_powercycle.is_power_util_available"
        ) as is_avail:
            is_avail.return_value = False
            req = await self.client.request(
                "POST",
                "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset",
                json={"ResetType": "On"},
            )
            self.assertEqual(
                await req.json(),
                {
                    "Response": "Mocking fru/server to turn on/off/reboot",
                    "status": "OK",
                },
            )
            self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_managers_powercycle_compute(self):
        req = await self.client.request(
            "POST",
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
            json={"ResetType": "ForceRestart"},
        )
        self.assertEqual(
            await req.json(),
            {"status": "OK"},
        )
        self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_managers_powercycle_fboss(self):
        req = await self.client.request(
            "POST",
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
            json={"ResetType": "ForceRestart"},
        )
        self.assertEqual(
            await req.json(),
            {"status": "OK"},
        )
        self.assertEqual(200, req.status)

    async def get_application(self) -> aiohttp.web.Application:
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        webapp.router.add_post(
            "/redfish/v1/Systems/{fru_name}/Actions/ComputerSystem.Reset",
            redfish_powercycle.powercycle_post_handler,
        )
        webapp.router.add_post(
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
            redfish_powercycle.oobcycle_post_handler,
        )
        return webapp
