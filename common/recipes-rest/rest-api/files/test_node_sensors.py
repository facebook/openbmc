import os
import subprocess
import unittest
from unittest.mock import patch

from node_sensors import sensorsNode


class TestSensors(unittest.TestCase):
    def setUp(self):
        pass

    @patch.object(subprocess, "check_output")
    def test_basic_call(self, mocked_check_output):
        mocked_check_output.return_value = b"MB_INLET_TEMP                (0xA0) :   33.31 C     | (ok)\nMB_OUTLET_TEMP               (0xA1) :   29.56 C     | (ok)"
        snr = sensorsNode("mb")
        expected_full_output = {
            "MB_INLET_TEMP": {"value": "33.31"},
            "MB_OUTLET_TEMP": {"value": "29.56"},
        }
        expected_filtered_output = {"MB_INLET_TEMP": {"value": "33.31"}}
        expected_status_output = {"MB_INLET_TEMP": {"value": "33.31", "status": "ok"}}
        expected_units_output = {"MB_INLET_TEMP": {"value": "33.31", "units": "C"}}
        # Get all sensors
        self.assertEqual(snr.getInformation(), expected_full_output)
        mocked_check_output.assert_called_with(["/usr/local/bin/sensor-util", "mb"])
        # Get sensor by name
        self.assertEqual(
            snr.getInformation(param={"name": "MB_INLET_TEMP"}),
            expected_filtered_output,
        )
        mocked_check_output.assert_called_with(["/usr/local/bin/sensor-util", "mb"])
        # Get unknown sensor by name
        self.assertEqual(snr.getInformation(param={"name": "MB_INLET_TEMP2"}), {})
        mocked_check_output.assert_called_with(["/usr/local/bin/sensor-util", "mb"])

        self.assertEqual(
            snr.getInformation(param={"name": "MB_INLET_TEMP", "display": "status"}),
            expected_status_output,
        )
        mocked_check_output.assert_called_with(["/usr/local/bin/sensor-util", "mb"])
        self.assertEqual(
            snr.getInformation(param={"name": "MB_INLET_TEMP", "display": "units"}),
            expected_units_output,
        )
        mocked_check_output.assert_called_with(["/usr/local/bin/sensor-util", "mb"])

    @patch.object(subprocess, "check_output")
    def test_filter_by_id_call(self, mocked_check_output):
        mocked_check_output.return_value = (
            b"MB_INLET_TEMP                (0xA0) :   33.31 C     | (ok)"
        )
        snr = sensorsNode("mb")
        expected_filtered_output = {"MB_INLET_TEMP": {"value": "33.31"}}
        # Get sensor by id.
        self.assertEqual(
            snr.getInformation(param={"id": "0xA0"}), expected_filtered_output
        )
        mocked_check_output.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "0xA0"]
        )
        # Get unknown sensor by id
        self.assertEqual(snr.getInformation(param={"id": "0xA9"}), {})
        mocked_check_output.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "0xA9"]
        )

    @patch.object(subprocess, "check_output")
    def test_thresholds_call(self, mocked_check_output):
        mocked_check_output.return_value = b"MB_INLET_TEMP                (0xA0) :   32.62 C     | (ok) | UCR: NA | UNC: NA | UNR: NA | LCR: NA | LNC: NA | LNR: NA\nMB_OUTLET_TEMP               (0xA1) :   29.25 C     | (ok) | UCR: 90.00 | UNC: NA | UNR: NA | LCR: NA | LNC: NA | LNR: NA"
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
            snr.getInformation(
                param={"name": "MB_INLET_TEMP", "display": "thresholds"}
            ),
            expected_no_thresholds_sensor,
        )
        mocked_check_output.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--threshold"]
        )
        self.assertEqual(
            snr.getInformation(
                param={"name": "MB_OUTLET_TEMP", "display": "thresholds,units"}
            ),
            expected_single_thresholds_sensor,
        )
        mocked_check_output.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--threshold"]
        )

    @patch.object(subprocess, "check_output")
    def test_history_call(self, mocked_check_output):
        mocked_check_output.return_value = b"MB_INLET_TEMP      (0xA0) min = 32.88, average = 32.88, max = 32.88\nMB_OUTLET_TEMP     (0xA1) min = 29.25, average = 29.31, max = 29.31"
        snr = sensorsNode("mb")
        expected_filtered_output = {
            "MB_INLET_TEMP": {"min": "32.88", "avg": "32.88", "max": "32.88"}
        }
        self.assertEqual(
            snr.getInformation(
                param={
                    "name": "MB_INLET_TEMP",
                    "display": "history",
                    "history-period": "70",
                }
            ),
            expected_filtered_output,
        )
        mocked_check_output.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--history", "70"]
        )

    @patch.object(subprocess, "check_call")
    def test_history_clear(self, mocked_check_call):
        snr_readonly = sensorsNode("mb")
        snr_readwrite = sensorsNode("mb", actions=["history-clear"])

        # Ensure that we do not do any actions which
        # we were not initialized with. Respect read-only
        self.assertEqual(
            snr_readonly.doAction({"action": "history-clear"}), {"result": "failure"}
        )
        mocked_check_call.assert_not_called()
        self.assertEqual(
            snr_readonly.doAction({"action": "history-clear"}), {"result": "failure"}
        )
        mocked_check_call.assert_not_called()

        # Ensure that we respect read-only even if
        # we support actions.
        self.assertEqual(
            snr_readwrite.doAction({"action": "history-clear"}), {"result": "failure"}
        )
        mocked_check_call.assert_not_called()

        # Ensure that we accept calls to clear history of all sensors
        self.assertEqual(
            snr_readwrite.doAction({"action": "history-clear"}, False),
            {"result": "success"},
        )
        mocked_check_call.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--history-clear"]
        )

        # Ensure we can clear history of specific sensor
        self.assertEqual(
            snr_readwrite.doAction({"action": "history-clear", "id": "0xA0"}, False),
            {"result": "success"},
        )
        mocked_check_call.assert_called_with(
            ["/usr/local/bin/sensor-util", "mb", "--history-clear", "0xA0"]
        )
