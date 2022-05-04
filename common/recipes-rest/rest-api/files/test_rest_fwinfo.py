import asyncio
import json
import unittest

import aiohttp
import rest_fwinfo
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler

EXAMPLE_BMC_FRUID_OBJ = [
    {
        "FRU Information": "BMC",
        "Board Mfg Date": "Wed Mar 31 05:23:00 2021",
        "Board Mfg": "BMC manufacturer",
        "Board Product": "BMC Storage Module",
        "Board Serial": "BMCSerial",
        "Board Part Number": "Part.number.42",
        "Board FRU ID": "N/A",
        "Board Custom Data 1": "01-1111111",
        "Board Custom Data 2": "RANDOM DATA",
        "Product Manufacturer": "BMC manufacturer",
        "Product Name": "Discovery Point V1",
        "Product Part Number": "This.is.BMC",
        "Product Version": "EVT",
        "Product Serial": "MYSERIAL",
        "Product Asset Tag": "N/A",
        "Product FRU ID": "N/A",
        "Product Custom Data 1": "01-1111111",
        "Product Custom Data 2": "RANDOM DATA",
    }
]
EXAMPLE_SLOT1_FRUID_OBJ = [
    {
        "FRU Information": "Server Board 1",
        "Chassis Type": "Rack Mount Chassis",
        "Chassis Part Number": "N/A",
        "Chassis Serial Number": "N/A",
        "Chassis Custom Data 1": "RANDOM DATA",
        "Board Mfg Date": "Mon Jan 21 04:41:00 2019",
        "Board Mfg": "Mighty manufacturer",
        "Board Product": "Twin Lakes Passive MP",
        "Board Serial": "MYSERIAL",
        "Board Part Number": "Part.number.42",
        "Board FRU ID": "N/A",
        "Board Custom Data 1": "02-000288",
        "Product Manufacturer": "Mighty manufacturer",
        "Product Part Number": "I.AM.A.PART",
        "Product Name": "Twin Lakes MP",
        "Product Version": "YoTL02",
        "Product Serial": "MYSERIAL",
        "Product Asset Tag": "N/A",
        "Product FRU ID": "N/A",
        "Product Custom Data 1": "01-003230",
        "Product Custom Data 2": "MP",
    }
]
EXAMPLE_NIC_FRUID_OBJ = [
    {
        "FRU Information": "NIC",
        "Board Mfg Date": "Thu Jan 24 18:06:00 2019",
        "Board Mfg": "NIC Manufacturer",
        "Board Product": "A very good nic",
        "Board Serial": "SERIAL-number-42",
        "Board Part Number": "PART_NUMBER_42",
        "Board FRU ID": "FRU Ver 0.01",
        "Board Custom Data 1": "N/A",
        "Board Custom Data 2": "N/A",
        "Board Custom Data 3": "N/A",
        "Product Manufacturer": "NIC Manufacturer",
        "Product Name": "A very good nic",
        "Product Part Number": "PART_NUMBER_42",
        "Product Version": "A3 ",
        "Product Serial": "SERIAL-number-42",
        "Product Asset Tag": "N/A",
        "Product FRU ID": "N/A",
        "Product Custom Data 1": "N/A",
        "Product Custom Data 2": "DVT",
        "Product Custom Data 3": "customcustom",
    }
]

EXAMPLE_FW_UTIL_STDOUT = """BMC Version: fby2-v2021.12.8\nBMC CPLD Version: CPLD_VERSION_42\n2OU Bridge-IC Version: BICBICBIC"""


class TestRestFwinfo(AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        asyncio.set_event_loop(asyncio.new_event_loop())

        # Python >= 3.8 smartly uses AsyncMock automatically if the target
        # is a coroutine. However, this breaks compatibility with older python versions,
        # so forcing new_callable=MagicMock to preserve backwards compatibility
        bmc_future = asyncio.Future()
        bmc_future.set_result((0, json.dumps(EXAMPLE_BMC_FRUID_OBJ), ""))
        slot1_future = asyncio.Future()
        slot1_future.set_result((0, json.dumps(EXAMPLE_SLOT1_FRUID_OBJ), ""))
        nic_future = asyncio.Future()
        nic_future.set_result((0, json.dumps(EXAMPLE_NIC_FRUID_OBJ), ""))
        nicexp_future = asyncio.Future()
        nicexp_future.set_result((255, "", ""))
        fw_util_future = asyncio.Future()
        fw_util_future.set_result((0, EXAMPLE_FW_UTIL_STDOUT, ""))
        self.async_exec_mock = unittest.mock.MagicMock(
            side_effect=[
                bmc_future,
                nic_future,
                nicexp_future,
                slot1_future,
                fw_util_future,
            ]
        )

        self.patches = [
            unittest.mock.patch("common_utils.async_exec", self.async_exec_mock),
        ]

        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    @unittest_run_loop
    async def test_get_fwinfo(self):
        expected_resp = {
            "Actions": [],
            "Information": {
                "fruid_info": {
                    "bmc": {
                        "model": "BMC Storage Module",
                        "part_number": "This.is.BMC",
                        "serial_number": "BMCSerial",
                        "vendor": "BMC manufacturer",
                    },
                    "nic": {
                        "model": "A very good nic",
                        "part_number": "PART_NUMBER_42",
                        "serial_number": "SERIAL-number-42",
                        "vendor": "NIC Manufacturer",
                    },
                    "nicexp": {
                        "model": "",
                        "part_number": "",
                        "serial_number": "",
                        "vendor": "",
                    },
                    "slot1": {
                        "model": "Twin Lakes Passive MP",
                        "part_number": "I.AM.A.PART",
                        "serial_number": "MYSERIAL",
                        "vendor": "Mighty manufacturer",
                    },
                },
                "fw_info": {
                    "bmc": {
                        "bmc_cpld_ver": " CPLD_VERSION_42",
                        "bmc_ver": " fby2-v2021.12.8",
                    }
                },
            },
            "Resources": [],
        }
        req = await self.client.request("GET", "/api/fwinfo")
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_fruidutil_and_fwinfo_calls_are_cached(self):
        await self.client.request("GET", "/api/fwinfo")
        await self.client.request("GET", "/api/fwinfo")
        self.assertEqual(self.async_exec_mock.call_count, 5)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        webapp.router.add_get("/api/fwinfo", rest_fwinfo.rest_fwinfo_handler)
        return webapp
