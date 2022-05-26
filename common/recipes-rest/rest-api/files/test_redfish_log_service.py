import asyncio
import unittest

import aiohttp
import common_utils
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler


FRU_NAME = "server1"
# Output of /usr/local/bin/log-util slot1 --print  --json
# on a machine I've been working on, real-world test data
MOCK_RESP = """
{
    "Logs": [
        {
            "APP_NAME": "gpiod",
            "FRU#": "1",
            "FRU_NAME": "slot1",
            "MESSAGE": "FRU: 1, System powered OFF",
            "TIME_STAMP": "2021-11-01 15:39:10"
        },
        {
            "APP_NAME": "gpiod",
            "FRU#": "1",
            "FRU_NAME": "slot1",
            "MESSAGE": "FRU: 1, System powered ON",
            "TIME_STAMP": "2021-11-10 02:43:39"
        },
        {
            "APP_NAME": "ipmid",
            "FRU#": "1",
            "FRU_NAME": "slot1",
            "MESSAGE": "SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2021-11-10 02:43:41, Sensor: SYSTEM_STATUS (0x10), Event Data: (07FFFF) Platform_Reset Deassertion ",
            "TIME_STAMP": "2021-11-10 02:43:41"
        }
    ]
}
"""  # noqa B950
MOCK_ASYNC_EXEC = (0, MOCK_RESP, None)


class TestGetLogService(AioHTTPTestCase):
    def setUp(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        self.patches = [
            unittest.mock.patch(
                "common_utils.async_exec",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=asyncio.Future(),
            ),
            unittest.mock.patch(
                "pal.pal_fru_name_map",
                create=True,
                return_value={"slot1": 1, "slot2": 2, "slot3": 3, "slot4": 4, "spb": 5},
            ),
            unittest.mock.patch(
                "redfish_log_service.is_log_util_available",
                create=True,
                return_value=True,
            ),
        ]

        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)
        common_utils.async_exec.return_value.set_result(MOCK_ASYNC_EXEC)
        super().setUp()

    @unittest_run_loop
    async def test_systems_get_log_services_root(self):
        req = await self.client.request(
            "GET",
            "/redfish/v1/Systems/{}".format(FRU_NAME) + "/LogServices",
        )
        expected = {
            "@odata.type": "#LogServiceCollection.LogServiceCollection",
            "Name": "Log Service Collection",
            "Description": "Collection of Logs for this System",
            "Members@odata.count": 1,
            "Members": [{"@odata.id": "/redfish/v1/Systems/server1/LogServices/SEL"}],
            "Oem": {},
            "@odata.id": "/redfish/v1/Systems/{}/LogServices".format(FRU_NAME),
        }
        response = await req.json()
        self.maxDiff = None
        self.assertEqual(response, expected)
        self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_systems_get_log_service_sel(self):
        req = await self.client.request(
            "GET", "/redfish/v1/Systems/{}".format(FRU_NAME) + "/LogServices/SEL"
        )

        expected = {
            "@odata.id": "/redfish/v1/Systems/{}".format(FRU_NAME) + "/LogServices/SEL",
            "@odata.type": "#LogService.v1_1_2.LogService",
            "Id": "SEL",
            "LogEntryType": "SEL",
            "Entries": {
                "@odata.id": "/redfish/v1/Systems/{}".format(FRU_NAME)
                + "/LogServices/SEL/Entries",
            },
        }

        response = await req.json()
        self.maxDiff = None
        self.assertEqual(response, expected)
        self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_systems_get_log_service_sel_entries(self):
        req = await self.client.request(
            "GET",
            "/redfish/v1/Systems/{}".format(FRU_NAME) + "/LogServices/SEL/Entries",
        )
        expected = {
            "@odata.id": "/redfish/v1/Systems/{}".format(FRU_NAME)
            + "/LogServices/SEL/Entries",
            "@odata.type": "#LogEntryCollection.LogEntryCollection",
            "Description": "Collection of entries for this log service",
            "Members": [
                {
                    "@odata.id": "/redfish/v1/Systems/{}".format(FRU_NAME)
                    + "/LogServices/SEL/Entries/1",
                    "@odata.type": "#LogEntry.v1_10_0.LogEntry",
                    "EventTimestamp": "2021-11-10 02:43:41",
                    "EntryType": "ipmid",
                    "Name": "slot1:1",
                    "Message": "SEL Entry: FRU: 1, "
                    + "Record: Standard (0x02), "
                    + "Time: 2021-11-10 02:43:41, "
                    + "Sensor: SYSTEM_STATUS (0x10), "
                    + "Event Data: (07FFFF) Platform_Reset Deassertion",
                },
            ],
            "Members@odata.count": 1,
        }
        response = await req.json()
        self.maxDiff = None
        self.assertEqual(response, expected)
        self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_systems_get_log_service_sel_entry(self):
        req = await self.client.request(
            "GET",
            "/redfish/v1/Systems/{}".format(FRU_NAME) + "/LogServices/SEL/Entries/1",
        )
        expected = {
            "@odata.id": "/redfish/v1/Systems/{}".format(FRU_NAME)
            + "/LogServices/SEL/Entries/1",
            "@odata.type": "#LogEntry.v1_10_0.LogEntry",
            "EventTimestamp": "2021-11-10 02:43:41",
            "EntryType": "ipmid",
            "Name": "slot1:1",
            "Message": "SEL Entry: FRU: 1, "
            + "Record: Standard (0x02), "
            + "Time: 2021-11-10 02:43:41, "
            + "Sensor: SYSTEM_STATUS (0x10), "
            + "Event Data: (07FFFF) Platform_Reset Deassertion",
        }
        response = await req.json()
        self.maxDiff = None
        self.assertEqual(response, expected)
        self.assertEqual(200, req.status)

    @unittest_run_loop
    async def test_systems_get_log_service_bad_logservice_id(self):
        req = await self.client.request(
            "GET",
            "/redfish/v1/Systems/{}".format(FRU_NAME)
            + "/LogServices/IWillNeverExist/Entries/1",
        )

        response = await req.text()
        expected = ""
        self.maxDiff = None
        self.assertEqual(response, expected)
        self.assertEqual(404, req.status)

    async def get_application(self):
        from redfish_log_service import RedfishLogService

        log_service = RedfishLogService().get_log_service_controller()
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])

        webapp.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices",
            log_service.get_log_services_root,
        )
        webapp.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}",
            log_service.get_log_service,
        )
        webapp.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}/Entries",
            log_service.get_log_service_entries,
        )
        webapp.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices"
            + "/{LogServiceID}/Entries/{EntryID}",
            log_service.get_log_service_entry,
        )
        return webapp
