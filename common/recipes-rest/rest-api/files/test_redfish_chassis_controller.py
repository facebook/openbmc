import asyncio
import sys
import types
import typing as t
import unittest

# workaround bc pal and sdr are unavailable in unit test envs
sys.modules["pal"] = types.ModuleType("pal")
sys.modules["sdr"] = types.ModuleType("sdr")
sys.modules["sensors"] = types.ModuleType("sensors")

import aiohttp.web
import pal
import redfish_chassis_helper
import sdr
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler


# mocking a tuple instead of sdr.ThreshSensor bc sdr lib isn't available here
sensor_thresh = t.NamedTuple(
    "sensor_thresh",
    [
        ("unc_thresh", float),
        ("ucr_thresh", float),
        ("unr_thresh", float),
        ("lnc_thresh", float),
        ("lcr_thresh", float),
        ("lnr_thresh", float),
    ],
)


class TestChassisService(AioHTTPTestCase):
    def setUp(self):
        asyncio.set_event_loop(asyncio.new_event_loop())

        # mocking a tuple instead of pal.SensorHistory bc pal lib isn't available here
        SensorHistory = t.NamedTuple(
            "SensorHistory",
            [
                ("min_intv_consumed", float),
                ("max_intv_consumed", float),
                ("avg_intv_consumed", float),
            ],
        )

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
                return_value={"slot1": 1, "slot2": 2, "slot3": 3, "slot4": 4, "spb": 5},
            ),
            unittest.mock.patch(
                "pal.pal_is_fru_prsnt",
                create=True,
                return_value=True,
            ),
            unittest.mock.patch(
                "pal.pal_get_fru_sensor_list", create=True, side_effect=[[224]]
            ),
            unittest.mock.patch(
                "sdr.sdr_get_sensor_units",
                create=True,
                side_effect=["Amps", "C", "RPM"],
            ),
            unittest.mock.patch(
                "pal.sensor_read",
                create=True,
                side_effect=[24.9444444444, 7177],
            ),
            unittest.mock.patch(
                "sdr.sdr_get_sensor_thresh",
                create=True,
                side_effect=[
                    sensor_thresh(
                        0,
                        5.5,
                        0,
                        0,
                        0,
                        0,
                    ),
                ],
            ),
            unittest.mock.patch(
                "pal.sensor_read_history",
                new_callable=unittest.mock.MagicMock,
                create=True,
                return_value=SensorHistory(5.02, 5.03, 5.03),
            ),
            unittest.mock.patch(
                "sdr.sdr_get_sensor_name",
                create=True,
                side_effect=["SP_P5V", "SP_INLET_TEMP", "SP_FAN0_TACH"],
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
            "Members@odata.count": 1,
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
                sdr.sdr_get_sensor_thresh.side_effect = [
                    sensor_thresh(
                        0,
                        5.5,
                        0,
                        0,
                        0,
                        0,
                    ),
                ]
                redfish_chassis_helper.get_fru_info.return_value = asyncio.Future()
                redfish_chassis_helper.get_fru_info.return_value.set_result(
                    redfish_chassis_helper.FruInfo(
                        fru_name, "Wiwynn", "WTL19121DSMA1", "Yosemite V2 MP"
                    )
                )

                expected_resp = {
                    "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
                    "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
                    "@odata.type": "#Chassis.v1_5_0.Chassis",
                    "Id": "1",
                    "Name": "Computer System Chassis",
                    "ChassisType": "RackMount",
                    "FruInfo": [
                        {
                            "FruName": fru_name,
                            "Manufacturer": "Wiwynn",
                            "Model": "Yosemite V2 MP",
                            "SerialNumber": "WTL19121DSMA1",
                        },
                    ],
                    "PowerState": "On",
                    "Status": {"State": "Enabled", "Health": "OK"},
                    "Thermal": {
                        "@odata.id": "/redfish/v1/Chassis/{}/Thermal".format(
                            server_name
                        )
                    },
                    "Power": {
                        "@odata.id": "/redfish/v1/Chassis/{}/Power".format(server_name)
                    },
                    "Links": {},
                }
                req = await self.client.request(
                    "GET", "/redfish/v1/Chassis/{}".format(server_name)
                )
                resp = await req.json()
                self.maxDiff = None
                self.assertEqual(resp, expected_resp)
                self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_chassis_thermal(self):
        "Testing thermal response for both single sled frus and multisled frus"

        for server_name in ["1", "server1", "server2", "server3", "server4"]:
            fru_name = self.get_fru_name(server_name)
            with self.subTest(server_name=server_name):
                pal.pal_get_fru_sensor_list.side_effect = [[129], [70]]
                sdr.sdr_get_sensor_units.side_effect = ["C", "RPM"]
                sdr.sdr_get_sensor_name.side_effect = [
                    "SP_INLET_TEMP",
                    "SP_FAN0_TACH",
                ]
                pal.sensor_read.side_effect = [24.9444444444, 7177]
                sdr.sdr_get_sensor_thresh.side_effect = [
                    sensor_thresh(
                        0,
                        40,
                        0,
                        0,
                        0,
                        0,
                    ),
                    sensor_thresh(
                        0,
                        70,
                        0,
                        0,
                        0,
                        0,
                    ),
                ]
                expected_resp = {
                    "Redundancy": [
                        {
                            "@odata.id": "/redfish/v1/Chassis/{}/Thermal#/Redundancy/0".format(  # noqa: B950
                                server_name
                            ),
                            "MemberId": "0",
                            "Name": "BaseBoard System Fans",
                            "RedundancySet": [
                                {
                                    "@odata.id": "/redfish/v1/Chassis/{}/Thermal#/Fans/0".format(  # noqa: B950
                                        server_name
                                    )
                                }
                            ],
                            "Mode": "N+m",
                            "Status": {"State": "Enabled", "Health": "OK"},
                        },
                    ],
                    "@odata.type": "#Thermal.v1_7_0.Thermal",
                    "@odata.id": "/redfish/v1/Chassis/{}/Thermal".format(server_name),
                    "Id": "Thermal",
                    "Temperatures": [
                        {
                            "@odata.id": "/redfish/v1/Chassis/{}/Thermal#/Temperatures/0".format(  # noqa: B950
                                server_name
                            ),
                            "FruName": fru_name,
                            "LowerThresholdNonCritical": 0,
                            "LowerThresholdCritical": 0,
                            "PhysicalContext": "Chassis",
                            "UpperThresholdCritical": 40,
                            "MemberId": 0,
                            "UpperThresholdNonCritical": 0,
                            "UpperThresholdFatal": 0,
                            "ReadingCelsius": 24.94,
                            "Name": "SP_INLET_TEMP",
                            "SensorNumber": 129,
                            "LowerThresholdFatal": 0,
                            "Status": {"Health": "OK", "State": "Enabled"},
                        },
                    ],
                    "Fans": [
                        {
                            "Name": "SP_FAN0_TACH",
                            "Reading": 7177,
                            "ReadingUnits": "RPM",
                            "SensorNumber": 70,
                            "MemberId": 0,
                            "Status": {"State": "Enabled", "Health": "OK"},
                            "FruName": fru_name,
                            "PhysicalContext": "Backplane",
                            "@odata.id": "/redfish/v1/Chassis/{}/Thermal#/Fans/0".format(  # noqa: B950
                                server_name
                            ),
                            "LowerThresholdFatal": 0,
                            "Redundancy": [
                                {
                                    "@odata.id": "/redfish/v1/Chassis/{}/Thermal#/Redundancy/0".format(  # noqa: B950
                                        server_name
                                    )
                                }
                            ],
                        },
                    ],
                    "Name": "Thermal",
                }
                req = await self.client.request(
                    "GET", "/redfish/v1/Chassis/{}/Thermal".format(server_name)
                )
                resp = await req.json()
                self.maxDiff = None
                self.assertEqual(resp, expected_resp)
                self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_chassis_power(self):
        "Testing power response for both single sled frus and multisled frus"
        for server_name in ["1", "server1", "server2", "server3", "server4"]:
            fru_name = self.get_fru_name(server_name)
            with self.subTest(server_name=server_name):
                pal.pal_get_fru_sensor_list.side_effect = [[224]]
                sdr.sdr_get_sensor_units.side_effect = ["Amps"]
                sdr.sdr_get_sensor_name.side_effect = ["SP_P5V"]
                pal.sensor_read.side_effect = [
                    0
                ]  # placeholder to avoid running out of side_effect vals
                sdr.sdr_get_sensor_thresh.side_effect = [
                    sensor_thresh(
                        0,
                        5.5,
                        0,
                        0,
                        0,
                        0,
                    ),
                ]
                expected_resp = {
                    "@odata.context": "/redfish/v1/$metadata#Power.Power",
                    "@odata.id": "/redfish/v1/Chassis/{}/Power".format(server_name),
                    "@odata.type": "#Power.v1_5_0.Power",
                    "Id": "Power",
                    "Name": "Power",
                    "PowerControl": [
                        [
                            {
                                "PowerLimit": {
                                    "LimitException": "LogEventOnly",
                                    "LimitInWatts": 5.5,
                                },
                                "PhysicalContext": "Chassis",
                                "@odata.id": "/redfish/v1/Chassis/{}/Power#/PowerControl/0".format(  # noqa: B950
                                    server_name
                                ),
                                "FruName": fru_name,
                                "SensorNumber": 224,
                                "Name": "SP_P5V",
                                "PowerMetrix": {
                                    "MaxIntervalConsumedWatts": 5.03,
                                    "MinIntervalConsumedWatts": 5.02,
                                    "IntervalInMin": 1,
                                    "AverageIntervalConsumedWatts": 5.03,
                                },
                                "MemberId": 0,
                            }
                        ],
                    ],
                }
                req = await self.client.request(
                    "GET", "/redfish/v1/Chassis/{}/Power".format(server_name)
                )
                resp = await req.json()
                self.maxDiff = None
                self.assertEqual(resp, expected_resp)
                self.assertEqual(req.status, 200)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.setup_redfish_common_routes(webapp)
        redfish.setup_multisled_routes(webapp)
        return webapp
