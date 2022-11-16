import asyncio
import subprocess
import unittest
from unittest.mock import Mock, patch

import aiohttp
import common_utils
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from node_sensors import sensorsNode


class TestSensors(AioHTTPTestCase):
    async def setUpAsync(self):
        asyncio.set_event_loop(asyncio.new_event_loop())

        self.async_exec_mock = unittest.mock.patch(
            "common_utils.async_exec",
            return_value=asyncio.Future(),
            autospec=True,
        )

        self.async_exec_mock.start()
        await super().setUpAsync()

    def tearDown(self):
        self.async_exec_mock.stop()

    async def get_application(self):
        return aiohttp.web.Application()

    @unittest_run_loop
    async def test_basic_call(self):
        return_value = "MB_INLET_TEMP                (0xA0) :   33.31 C     | (ok)\nMB_OUTLET_TEMP               (0xA1) :   29.56 C     | (ok)"
        common_utils.async_exec.return_value.set_result((0, return_value, ""))
        snr = sensorsNode("mb")
        expected_full_output = {
            "MB_INLET_TEMP": {"value": "33.31"},
            "MB_OUTLET_TEMP": {"value": "29.56"},
        }
        expected_filtered_output = {"MB_INLET_TEMP": {"value": "33.31"}}
        expected_status_output = {"MB_INLET_TEMP": {"value": "33.31", "status": "ok"}}
        expected_units_output = {"MB_INLET_TEMP": {"value": "33.31", "units": "C"}}
        # Get all sensors
        self.assertEqual(await snr.getInformation(param={}), expected_full_output)
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb"],
            shell=False,
        )
        # Get sensor by name
        self.assertEqual(
            await snr.getInformation(param={"name": "MB_INLET_TEMP"}),
            expected_filtered_output,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb"],
            shell=False,
        )
        # Get unknown sensor by name
        self.assertEqual(await snr.getInformation(param={"name": "MB_INLET_TEMP2"}), {})
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb"],
            shell=False,
        )

        self.assertEqual(
            await snr.getInformation(
                param={"name": "MB_INLET_TEMP", "display": "status"}
            ),
            expected_status_output,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb"],
            shell=False,
        )
        self.assertEqual(
            await snr.getInformation(
                param={"name": "MB_INLET_TEMP", "display": "units"}
            ),
            expected_units_output,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb"],
            shell=False,
        )

    @unittest_run_loop
    async def test_filter_by_id_call(self):
        return_value = "MB_INLET_TEMP                (0xA0) :   33.31 C     | (ok)"
        common_utils.async_exec.return_value.set_result((0, return_value, ""))

        snr = sensorsNode("mb")
        expected_filtered_output = {"MB_INLET_TEMP": {"value": "33.31"}}
        # Get sensor by id.
        self.assertEqual(
            await snr.getInformation(param={"id": "0xA0"}),
            expected_filtered_output,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "0xA0"],
            shell=False,
        )
        # Get unknown sensor by id
        self.assertEqual(await snr.getInformation(param={"id": "0xA9"}), {})
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "0xA9"],
            shell=False,
        )

    @unittest_run_loop
    async def test_thresholds_call(self):
        return_value = "MB_INLET_TEMP                (0xA0) :   32.62 C     | (ok) | UCR: NA | UNC: NA | UNR: NA | LCR: NA | LNC: NA | LNR: NA\nMB_OUTLET_TEMP               (0xA1) :   29.25 C     | (ok) | UCR: 90.00 | UNC: NA | UNR: NA | LCR: NA | LNC: NA | LNR: NA"
        common_utils.async_exec.return_value.set_result((0, return_value, ""))

        snr = sensorsNode("mb")
        expected_no_thresholds_sensor = {"MB_INLET_TEMP": {"value": "32.62"}}
        expected_single_thresholds_sensor = {
            "MB_OUTLET_TEMP": {
                "thresholds": {"UCR": "90.00"},
                "value": "29.25",
                "units": "C",
            }
        }
        self.assertEqual(
            await snr.getInformation(
                param={"name": "MB_INLET_TEMP", "display": "thresholds"}
            ),
            expected_no_thresholds_sensor,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--threshold"],
            shell=False,
        )
        self.assertEqual(
            await snr.getInformation(
                param={"name": "MB_OUTLET_TEMP", "display": "thresholds,units"}
            ),
            expected_single_thresholds_sensor,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--threshold"],
            shell=False,
        )

    @unittest_run_loop
    async def test_history_call(self):
        return_value = "MB_INLET_TEMP      (0xA0) min = 32.88, average = 32.88, max = 32.88\nMB_OUTLET_TEMP     (0xA1) min = 29.25, average = 29.31, max = 29.31"
        common_utils.async_exec.return_value.set_result((0, return_value, ""))

        snr = sensorsNode("mb")
        expected_filtered_output = {
            "MB_INLET_TEMP": {"min": "32.88", "avg": "32.88", "max": "32.88"}
        }
        self.assertEqual(
            await snr.getInformation(
                param={
                    "name": "MB_INLET_TEMP",
                    "display": "history",
                    "history-period": "70",
                }
            ),
            expected_filtered_output,
        )
        common_utils.async_exec.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--history", "70"],
            shell=False,
        )
