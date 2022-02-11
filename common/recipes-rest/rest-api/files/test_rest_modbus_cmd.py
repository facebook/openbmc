import asyncio
import unittest

import aiohttp.web
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
                "rest_modbus_cmd.raw_modbus_command.get_response",
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

        rest_modbus_cmd.raw_modbus_command.get_response.return_value.set_result(
            EXAMPLE_MODBUS_RESPONSE
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
        self.assertRegex(resp["details"], "Invalid payload: Schema mismatch in .*")

        self.assertFalse(rest_modbus_cmd.SolitonBeamFlock.return_value.entered)

    @unittest_run_loop
    async def test_post_modbus_cmd_400_bad_commands(self):
        req = await self.client.request(
            "POST", "/api/sys/modbus/cmd", json={**EXAMPLE_PAYLOAD, "commands": [[164]]}
        )
        resp = await req.json()
        self.assertEqual(req.status, 400)
        self.assertEqual(
            resp,
            {
                "status": "Bad Request",
                "details": "Invalid payload: Expected at least two bytes in .commands[0] ([164])",  # noqa: B950
            },
        )

        self.assertFalse(rest_modbus_cmd.SolitonBeamFlock.return_value.entered)

    def test_validate_payload_schema_invalid_payloads(self):
        for payload in EXAMPLE_INVALID_PAYLOADS:
            with self.assertRaises(
                ValueError,
                msg="Expected payload {payload} to raise ValueError".format(
                    payload=repr(payload)
                ),
            ) as cm:
                rest_modbus_cmd._validate_payload_schema(
                    payload=payload, schema=rest_modbus_cmd.PAYLOAD_SCHEMA
                )

            self.assertIn("Schema mismatch in ", str(cm.exception))

    def test_validate_payload_schema_invalid_payload_msg(self):
        with self.assertRaises(ValueError) as cm:
            # Use invalid format in .commands to check the exception message
            rest_modbus_cmd._validate_payload_schema(
                payload={**EXAMPLE_PAYLOAD, "commands": ["a40300800001"]},
                schema=rest_modbus_cmd.PAYLOAD_SCHEMA,
            )

        self.assertEqual(
            str(cm.exception),
            "Schema mismatch in .commands[]: expected value ('a40300800001') to match schema [<class 'int'>]",  # noqa: B950
        )

    def test_validate_payload_schema_valid_payload(self):
        rest_modbus_cmd._validate_payload_schema(
            payload=EXAMPLE_PAYLOAD, schema=rest_modbus_cmd.PAYLOAD_SCHEMA
        )

    def test_validate_payload_commands_simple(self):
        # Should not raise ValueError
        rest_modbus_cmd._validate_payload_commands([[164, 3]])

    def test_validate_payload_commands_empty(self):
        with self.assertRaises(ValueError) as cm:
            rest_modbus_cmd._validate_payload_commands([])

        self.assertEqual(
            str(cm.exception), "Expected at least one command in .commands[]"
        )

    def test_validate_payload_commands_not_enough_commands(self):
        with self.assertRaises(ValueError) as cm:
            rest_modbus_cmd._validate_payload_commands([[164]])

        self.assertEqual(
            str(cm.exception), "Expected at least two bytes in .commands[0] ([164])"
        )

    def test_validate_payload_commands_out_of_range(self):
        with self.assertRaises(ValueError) as cm:
            rest_modbus_cmd._validate_payload_commands([[164, -1]])

        self.assertEqual(
            str(cm.exception), "Byte value out of range in .commands[0] ([164, -1])"
        )

        with self.assertRaises(ValueError) as cm:
            rest_modbus_cmd._validate_payload_commands([[164, 256]])

        self.assertEqual(
            str(cm.exception), "Byte value out of range in .commands[0] ([164, 256])"
        )

    def test_validate_payload_commands_not_allowed(self):
        for opcode in range(0, 0x100):
            if opcode in rest_modbus_cmd.ALLOWED_OPCODES:
                continue

            with self.assertRaises(ValueError) as cm:
                rest_modbus_cmd._validate_payload_commands([[164, opcode]])

            self.assertEqual(
                str(cm.exception),
                "Command opcode 0x{opcode:02x} ({opcode}) is not allowed in .commands[0][1] ([164, {opcode}])".format(  # noqa: B950
                    opcode=opcode,
                ),
            )

    def test_raw_modbus_command_get_response_timeout(self):
        with unittest.mock.patch("socket.socket", autospec=True) as p:
            client = p.return_value

            cmd = rest_modbus_cmd.raw_modbus_command(
                data=[1, 2, 3], expected_response_length=456, custom_timeout=789
            )

            client.recv.return_value = b"\xf1\xf0"

            resp = cmd._get_response()

            self.assertEqual(resp, [0xF1, 0xF0])

            # Check if timeout was set as expected
            client.settimeout.assert_called_once_with(
                (
                    rest_modbus_cmd.raw_modbus_command.RACKMOND_MIN_SOCKET_TIMEOUT_MS
                    + 789
                )
                / 1000
            )

    async def get_application(self):
        webapp = aiohttp.web.Application()
        webapp.router.add_post("/api/sys/modbus/cmd", rest_modbus_cmd.post_modbus_cmd)
        return webapp


EXAMPLE_PAYLOAD = {
    "commands": [[164, 3, 0, 128, 0, 1]],
    "expected_response_length": 0,
    "custom_timeout": 2,
}

EXAMPLE_MODBUS_RESPONSE = [0x7, 0x0, 0xA4, 0x3, 0x2, 0x49, 0x76, 0x42, 0x2B]

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
]


class MockAsyncContextManager:
    def __init__(self):
        self.entered = False
        self.exited = False

    async def __aenter__(self):
        self.entered = True

    async def __aexit__(self, exc_type, exc, tb):
        self.exited = True
