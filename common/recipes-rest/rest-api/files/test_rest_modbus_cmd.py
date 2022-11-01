import asyncio
import unittest
from typing import Any, List

import aiohttp.web

import jsonschema_lite as jsonschema
import pyrmd
import rest_modbus_cmd
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop


class TestRestModbusCmd(AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        asyncio.set_event_loop(asyncio.new_event_loop())

        # Python >= 3.8 smartly uses AsyncMock automatically if the target
        # is a coroutine. However, this breaks compatibility with older python versions,
        # so forcing new_callable=MagicMock to preserve backwards compatibility
        self.patches = [
            unittest.mock.patch(
                "pyrmd.RackmonAsyncInterface.raw",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=asyncio.Future(),
            ),
            unittest.mock.patch(
                "rest_modbus_cmd.SolitonBeamFlock",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=MockAsyncContextManager(),
            ),
        ]

        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)
        pyrmd.RackmonAsyncInterface.raw.return_value.set_result(
            EXAMPLE_MODBUS_RESPONSE[2:]
        )

    @unittest_run_loop
    async def test_post_modbus_cmd_200(self):
        req = await self.client.request(
            "POST", "/api/sys/modbus/cmd", json=EXAMPLE_PAYLOAD
        )

        resp = await req.json()

        self.assertEqual(req.status, 200)
        self.assertEqual(resp, {"status": "OK", "responses": [EXAMPLE_MODBUS_RESPONSE]})

        self.assertTrue(rest_modbus_cmd.SolitonBeamFlock.return_value.entered)
        self.assertTrue(rest_modbus_cmd.SolitonBeamFlock.return_value.exited)

    @unittest_run_loop
    async def test_post_modbus_cmd_400_bad_schema(self):
        req = await self.client.request(
            "POST", "/api/sys/modbus/cmd", json=EXAMPLE_INVALID_PAYLOADS[0]
        )
        resp = await req.json()
        self.assertEqual(req.status, 400)
        self.assertEqual(resp["status"], "Bad Request")

        self.assertFalse(rest_modbus_cmd.SolitonBeamFlock.return_value.entered)

    @unittest_run_loop
    async def test_post_modbus_cmd_400_bad_commands(self):
        req = await self.client.request(
            "POST", "/api/sys/modbus/cmd", json={**EXAMPLE_PAYLOAD, "commands": [[164]]}
        )
        resp = await req.json()
        self.assertEqual(req.status, 400)
        self.assertEqual(resp["status"], "Bad Request")

        self.assertFalse(rest_modbus_cmd.SolitonBeamFlock.return_value.entered)

    def test_validate_payload_schema_invalid_payloads(self):
        for payload in EXAMPLE_INVALID_PAYLOADS:
            with self.assertRaises(
                jsonschema.ValidationError,
                msg="Expected payload {payload} to raise ValidationError".format(
                    payload=repr(payload)
                ),
            ) as _:
                jsonschema.validate(payload, rest_modbus_cmd.MODBUS_CMD_PAYLOAD_SCHEMA)

    def test_validate_payload_schema_invalid_payload_msg(self):
        with self.assertRaises(jsonschema.ValidationError) as _:
            # Use invalid format in .commands to check the exception message
            jsonschema.validate(
                {**EXAMPLE_PAYLOAD, "commands": ["a40300800001"]},
                rest_modbus_cmd.MODBUS_CMD_PAYLOAD_SCHEMA,
            )

    def test_validate_payload_schema_valid_payload(self):
        jsonschema.validate(EXAMPLE_PAYLOAD, rest_modbus_cmd.MODBUS_CMD_PAYLOAD_SCHEMA)

    def test_validate_payload_commands_simple(self):
        # Should not raise ValidationError
        jsonschema.validate([164, 3], rest_modbus_cmd.COMMAND_SCHEMA)

    def test_validate_payload_commands_empty(self):
        with self.assertRaises(jsonschema.ValidationError) as _:
            # Use invalid format in .commands to check the exception message
            jsonschema.validate(
                {**EXAMPLE_PAYLOAD, "commands": []},
                rest_modbus_cmd.MODBUS_CMD_PAYLOAD_SCHEMA,
            )

    def test_validate_payload_commands_not_enough_commands(self):
        with self.assertRaises(jsonschema.ValidationError) as _:
            jsonschema.validate([164], rest_modbus_cmd.COMMAND_SCHEMA)

    def test_validate_payload_commands_out_of_range(self):
        with self.assertRaises(jsonschema.ValidationError) as _:
            jsonschema.validate([164, -1], rest_modbus_cmd.COMMAND_SCHEMA)

        with self.assertRaises(jsonschema.ValidationError) as _:
            jsonschema.validate([164, 256], rest_modbus_cmd.COMMAND_SCHEMA)

    def test_validate_payload_commands_not_allowed(self):
        for opcode in range(0, 0x100):
            if opcode in rest_modbus_cmd.ALLOWED_OPCODES:
                continue

            with self.assertRaises(jsonschema.ValidationError) as _:
                jsonschema.validate([164, opcode], rest_modbus_cmd.COMMAND_SCHEMA)

    async def get_application(self):
        webapp = aiohttp.web.Application()
        webapp.router.add_post("/api/sys/modbus/cmd", rest_modbus_cmd.post_modbus_cmd)
        return webapp


EXAMPLE_PAYLOAD = {
    "commands": [[164, 3, 0, 128, 0, 1]],
    "expected_response_length": 5,
    "custom_timeout": 2,
}

EXAMPLE_MODBUS_RESPONSE = [
    0x7,
    0x0,
    0xA4,
    0x3,
    0x2,
    0x49,
    0x76,
    0x42,
    0x2B,
]  # type: List[int]

EXAMPLE_INVALID_PAYLOADS = [
    [],
    None,
    "a",
    0,
    {},
    {"commands": []},  # missing fields
    {"commands": [], "expected_response_length": 0},  # missing fields
    {
        "commands": [],
        "expected_response_length": 0,
        "custom_timeot": 3,
    },  # misspelt field
    {
        "commands": [],
        "expected_response_length": 0,
        "custom_timeout": 3,
        "blah": 5,
    },  # extra field
    {
        "commands": [],
        "expected_response_length": 0,
        "custom_timeout": None,
    },  # wrong type
    {
        "commands": "a0",
        "expected_response_length": 0,
        "custom_timeout": 2,
    },  # wrong type
    {
        "commands": {},
        "expected_response_length": 0,
        "custom_timeout": 2,
    },  # wrong type
]  # type: List[Any]


class MockAsyncContextManager:
    def __init__(self):
        self.entered = False
        self.exited = False

    async def __aenter__(self):
        self.entered = True

    async def __aexit__(self, exc_type, exc, tb):
        self.exited = True
