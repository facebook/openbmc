import json
import os
import tempfile
import unittest

import aiohttp.web
import rest_fscd_sensor_data
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop


class TestRestFscdSensorData(AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        self._setup_mock_destination_file_path()

        self.patches = [
            unittest.mock.patch(
                "rest_fscd_sensor_data._validate_payload",
                wraps=rest_fscd_sensor_data._validate_payload,
            ),
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    def test_example_payload_matches_schema(self):
        rest_fscd_sensor_data._validate_payload(
            payload=EXAMPLE_PAYLOAD, schema=rest_fscd_sensor_data.PAYLOAD_SCHEMA
        )

    @unittest_run_loop
    async def test_post_fscd_sensor_data(self):
        # Call post_fscd_sensor_data with EXAMPLE_PAYLOAD
        resp = await self.client.request(
            "POST", "/api/sys/fscd_sensor_data", json=EXAMPLE_PAYLOAD
        )

        self.assertEqual(resp.status, 200)

        resp_json = await resp.json()
        self.assertEqual(resp_json, {"status": "OK"})

        # Check if _validate_payload was called
        rest_fscd_sensor_data._validate_payload.assert_any_call(
            payload=EXAMPLE_PAYLOAD, schema=rest_fscd_sensor_data.PAYLOAD_SCHEMA
        )

        # Check if DESTINATION_FILE_PATH was written with EXAMPLE_PAYLOAD
        self.assertTrue(os.path.exists(rest_fscd_sensor_data.DESTINATION_FILE_PATH))
        with open(rest_fscd_sensor_data.DESTINATION_FILE_PATH) as f:
            destination_json = json.load(f)

        self.assertEqual(EXAMPLE_PAYLOAD, destination_json)

    @unittest_run_loop
    async def test_post_fscd_sensor_data_bad_request_invalid_json(self):
        resp = await self.client.request(
            "POST", "/api/sys/fscd_sensor_data", json={"blah": 1}
        )

        # Return 400 on invalid input
        self.assertEqual(resp.status, 400)

        resp_json = await resp.json()
        self.assertEqual(
            resp_json,
            {
                "status": "Bad Request",
                "details": "Invalid JSON payload: Schema mismatch in .: expected value ({{'blah': 1}}) to match schema {PAYLOAD_SCHEMA}".format(  # noqa: B950
                    PAYLOAD_SCHEMA=rest_fscd_sensor_data.PAYLOAD_SCHEMA
                ),
            },
        )
        self.assertFalse(os.path.exists(rest_fscd_sensor_data.DESTINATION_FILE_PATH))

    @unittest_run_loop
    async def test_post_fscd_sensor_data_bad_request_non_json(self):
        resp = await self.client.request(
            "POST", "/api/sys/fscd_sensor_data", data=b"asd;"
        )

        # Return 400 on invalid input
        self.assertEqual(resp.status, 400)

        resp_json = await resp.json()
        self.assertEqual(
            resp_json,
            {
                "status": "Bad Request",
                "details": "Invalid JSON payload: Expecting value: line 1 column 1 (char 0)",  # noqa: B950
            },
        )
        self.assertFalse(os.path.exists(rest_fscd_sensor_data.DESTINATION_FILE_PATH))

    def test_validate_payload_empty(self):
        rest_fscd_sensor_data._validate_payload(payload={}, schema={})

    def test_validate_payload_simple(self):
        rest_fscd_sensor_data._validate_payload(payload=1, schema=int)
        rest_fscd_sensor_data._validate_payload(payload=1.0, schema=float)
        rest_fscd_sensor_data._validate_payload(payload="", schema=str)
        rest_fscd_sensor_data._validate_payload(payload={"a": 1}, schema={"a": int})
        rest_fscd_sensor_data._validate_payload(
            payload={"a": {"b": 1.0}}, schema={"a": {"b": float}}
        )

    def test_validate_payload_mismatch(self):
        with self.assertRaises(ValueError) as cm:
            rest_fscd_sensor_data._validate_payload(payload={"a": 1}, schema={"a": str})

        self.assertEqual(
            cm.exception.args[0],
            "Schema mismatch in .a: expected value (1) to match schema <class 'str'>",
        )

    ## Utils
    def _setup_mock_destination_file_path(self):
        mock_destination_dir = tempfile.TemporaryDirectory()  # noqa: P201
        self.addCleanup(mock_destination_dir.cleanup)

        mock_destination_file_path = unittest.mock.patch(
            "rest_fscd_sensor_data.DESTINATION_FILE_PATH",
            mock_destination_dir.name + "/incoming_fscd_sensor_data.json",
        )
        mock_destination_file_path.start()
        self.addCleanup(mock_destination_file_path.stop)

    async def get_application(self):
        app = aiohttp.web.Application()
        app.router.add_post(
            "/api/sys/fscd_sensor_data", rest_fscd_sensor_data.post_fscd_sensor_data
        )
        return app


EXAMPLE_PAYLOAD = {
    "global_ts": 1604676343,
    "data": {
        "th4_1_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_1_temp": {"value": 68.7, "ts": 1604676372},
        "qsfp_2_temp": {"value": 70.7, "ts": 1604676372},
        "qsfp_3_temp": {"value": 72.3, "ts": 1604676372},
        "qsfp_4_temp": {"value": 71.6, "ts": 1604676372},
        "qsfp_5_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_6_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_7_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_8_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_9_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_10_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_11_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_12_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_13_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_14_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_15_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_16_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_17_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_18_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_19_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_20_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_21_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_22_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_23_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_24_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_25_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_26_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_27_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_28_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_29_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_30_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_31_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_32_temp": {"value": 69.7, "ts": 1604676372},
    },
}
