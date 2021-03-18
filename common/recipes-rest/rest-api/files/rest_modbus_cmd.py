import asyncio
import fcntl
import pathlib
import socket
import struct
import typing as t
from typing import List

import aiohttp.web

FLOCK_SOLITON_BEAM = "/tmp/modbus_dynamo_solitonbeam.lock"
RACKMOND_SOCKET = "/var/run/rackmond.sock"
RACKMOND_MAX_RESPONSE_LEN = 4096

ALLOWED_OPCODES = (
    0x03,  # Read Holding Register (required by spec)
    0x04,  # Read Input Registers (required by spec)
    0x06,  # Write Single Register (required by spec)
    0x10,  # Write Multiple Registers
    0x14,  # Read File Record
    0x15,  # Write File Record
    0x16,  # Bit Masked Write Register
    0x17,  # Read/Write Multiple Registers
    0x2B,  # Encapsulated Transport
    0x41,  # Vendor Defined
    0x42,  # Vendor Defined
    0x43,  # Vendor Defined
    0x44,  # Vendor Defined
    0x45,  # Vendor Defined
    0x46,  # Vendor Defined
    0x47,  # Vendor Defined
    0x48,  # Vendor Defined
)


async def post_modbus_cmd(request: aiohttp.web.Request) -> aiohttp.web.Response:
    try:
        payload = await request.json()
        _validate_payload_schema(payload=payload, schema=PAYLOAD_SCHEMA)
        _validate_payload_commands(payload["commands"])

    except ValueError as e:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Invalid payload: " + str(e),
            },
            status=400,
        )

    responses = []
    # Get soliton beam lock so we don't race with it or modbuscmd
    async with SolitonBeamFlock():
        for cmd in payload["commands"]:
            modbus_cmd = raw_modbus_command(
                data=cmd,
                expected_response_length=payload["expected_response_length"],
                custom_timeout=payload["custom_timeout"],
            )

            # Not using asyncio.gather() here as, assuming by the need of flock,
            # it probably doesn't support parallel access patterns well
            responses.append(await modbus_cmd.get_response())

    return aiohttp.web.json_response(
        {
            "status": "OK",
            "responses": responses,
        }
    )


## Utils
def to_bytes(cmd: t.List[int]) -> bytes:
    return b"".join(i.to_bytes(1, "little") for i in cmd)


class raw_modbus_command:  # from common/recipes-core/rackmon/rackmon/rackmond.h
    COMMAND_TYPE_RAW_MODBUS = 1

    # Minimum timeout for reading rackmond socket. This is not the modbus
    # command timeout (which is user-defined in the custom_timeout field)
    RACKMOND_MIN_SOCKET_TIMEOUT_MS = 5000

    def __init__(
        self,
        data: t.List[int],
        expected_response_length: int = 0,
        custom_timeout: int = 0,  # ms
    ):
        self.custom_timeout = custom_timeout
        self._packed = (
            struct.pack(
                "HxxHHL",
                self.COMMAND_TYPE_RAW_MODBUS,
                len(data),
                expected_response_length,
                custom_timeout,
            )
            + to_bytes(data)
        )

    async def get_response(self) -> t.List[int]:
        # Read response inside thread executor so we don't hold the event loop
        return await asyncio.get_event_loop().run_in_executor(None, self._get_response)

    def _get_response(self) -> t.List[int]:
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.settimeout(
            (self.RACKMOND_MIN_SOCKET_TIMEOUT_MS + self.custom_timeout) / 1000
        )
        try:
            client.connect(RACKMOND_SOCKET)
            client.send(len(self._packed).to_bytes(2, "little"))
            client.send(self._packed)

            return list(client.recv(RACKMOND_MAX_RESPONSE_LEN))

        finally:
            client.close()


class SolitonBeamFlock:
    async def __aenter__(self):
        self.lock_file = self._open_lock_file()

        # Call flock inside thread executor so we don't hold the event loop
        await asyncio.get_event_loop().run_in_executor(
            None, fcntl.flock, self.lock_file.fileno(), fcntl.LOCK_EX
        )

    async def __aexit__(self, exc_type, exc, tb):
        self.lock_file.close()

    @staticmethod
    def _open_lock_file():
        try:
            return open(FLOCK_SOLITON_BEAM)
        except FileNotFoundError:
            pathlib.Path(FLOCK_SOLITON_BEAM).touch()
            return open(FLOCK_SOLITON_BEAM)


## Validation logic
def _validate_payload_schema(payload, schema, path=""):
    if type(schema) == type(payload) == dict and schema.keys() == payload.keys():
        for key, subschema in schema.items():
            _validate_payload_schema(
                payload=payload[key], schema=subschema, path=path + "." + key
            )

    elif type(schema) == type(payload) == list:
        for value in payload:
            _validate_payload_schema(payload=value, schema=schema[0], path=path + "[]")

    elif not _schema_match(payload, schema):
        raise ValueError(
            "Schema mismatch in {path}: expected value ({x}) to match schema {y}".format(  # noqa: B950
                path=path or ".", x=repr(payload), y=repr(schema)
            )
        )


def _schema_match(payload, schema):
    if type(payload) == type(schema) == dict:
        return schema.keys() == payload.keys()

    elif type(schema) == type:
        return type(payload) == schema

    elif type(schema) == str:
        return payload == schema

    return False


def _validate_payload_commands(commands: List[str]) -> None:
    if not commands:
        raise ValueError("Expected at least one command in .commands[]")

    for i, cmd in enumerate(commands):
        if len(cmd) < 2:
            raise ValueError(
                "Expected at least two bytes in .commands[{i}] ({cmd})".format(
                    i=i, cmd=repr(cmd)
                )
            )

        if not all(0 <= x <= 0xFF for x in cmd):
            raise ValueError(
                "Byte value out of range in .commands[{i}] ({cmd})".format(
                    i=i, cmd=repr(cmd)
                )
            )

        if cmd[1] not in ALLOWED_OPCODES:
            raise ValueError(
                "Command opcode 0x{cmd_1:02x} ({cmd_1}) is not allowed in .commands[{i}][1] ({cmd})".format(  # noqa: B950
                    i=i, cmd_1=cmd[1], cmd=repr(cmd)
                )
            )


## Schemas

# Each command is a list of integers with modbus opcodes
# e.g. [164, 3, 0, 128, 0, 1] == [0xa4, 0x03, 0x00, 0x80, 0x00, 0x01]
COMMAND_SCHEMA = [int]

PAYLOAD_SCHEMA = {
    # .commands accepts a list of commands
    "commands": [COMMAND_SCHEMA],
    "expected_response_length": int,
    # Timeout for each command (not total timeout)
    "custom_timeout": int,
}
