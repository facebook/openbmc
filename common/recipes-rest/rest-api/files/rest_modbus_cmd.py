import asyncio
import fcntl
import pathlib

import jsonschema_lite as jsonschema

try:
    import pyrmd
except ImportError:
    pyrmd = None

import aiohttp.web

FLOCK_SOLITON_BEAM = "/tmp/modbus_dynamo_solitonbeam.lock"
RACKMOND_SOCKET = "/var/run/rackmond.sock"
RACKMOND_MAX_RESPONSE_LEN = 4096

ALLOWED_OPCODES = [
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
]


# Convert the response received by pyrmd to a response
# which would have been returned by rackmon v1:
# [LEN_LOW_BYTE, LEN_HIGH_BYTE, <actual modbus response>]
# If an error occurred, the LEN would be set to zero
# and the 16bit error code (again low byte followed by
# high byte). This is unfortunate internals being exposed,
# but backwards compatibility is important.
def convert_response(resp):
    len_b = len(resp)
    return [len_b & 0xFF, (len_b >> 8) & 0xFF] + resp


async def post_modbus_cmd(request: aiohttp.web.Request) -> aiohttp.web.Response:
    if pyrmd is None:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Unsupported on current configuration",
            },
            status=400,
        )
    try:
        payload = await request.json()
        jsonschema.validate(payload, MODBUS_CMD_PAYLOAD_SCHEMA)

    except jsonschema.ValidationError as e:
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
            try:
                response = await pyrmd.RackmonAsyncInterface.raw(
                    cmd,
                    payload["expected_response_length"],
                    payload["custom_timeout"],
                    True,
                )
                responses.append(convert_response(response))
            except pyrmd.ModbusTimeout:
                responses.append([0, 4])
            except pyrmd.ModbusCRCError:
                responses.append([0, 5])
            except pyrmd.ModbusUnknownError:
                responses.append([0, 3])
            except Exception:
                responses.append([0, 1])

    return aiohttp.web.json_response(
        {
            "status": "OK",
            "responses": responses,
        }
    )


async def post_modbus_read(request: aiohttp.web.Request) -> aiohttp.web.Response:
    if pyrmd is None:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Unsupported on current configuration",
            },
            status=400,
        )
    try:
        payload = await request.json()
        jsonschema.validate(payload, READ_SCHEMA)

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
        for cmd in payload["req"]:
            try:
                response = await pyrmd.RackmonAsyncInterface.read(
                    cmd["devAddress"],
                    cmd["regAddress"],
                    cmd["numRegisters"],
                    cmd.get("timeout", 0),
                )
                responses.append({"status": "SUCCESS", "regValues": response})
            except pyrmd.ModbusException as e:
                responses.append({"status": str(e)})
            except Exception:
                responses.append({"status": "ERR_IO_FAILURE"})

    return aiohttp.web.json_response(responses)


async def post_modbus_write(request: aiohttp.web.Request) -> aiohttp.web.Response:
    if pyrmd is None:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Unsupported on current configuration",
            },
            status=400,
        )
    try:
        payload = await request.json()
        jsonschema.validate(payload, WRITE_SCHEMA)

    except ValueError as e:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Invalid payload: " + str(e),
            },
            status=400,
        )

    cmd = payload["req"]
    responses = []
    # Get soliton beam lock so we don't race with it or modbuscmd
    async with SolitonBeamFlock():
        for cmd in payload["req"]:
            try:
                await pyrmd.RackmonAsyncInterface.write(
                    cmd["devAddress"],
                    cmd["regAddress"],
                    cmd["regValue"],
                    cmd.get("timeout", 0),
                )
                responses.append("SUCCESS")
            except pyrmd.ModbusException as e:
                responses.append(str(e))
            except Exception:
                responses.append("ERR_IO_FAILURE")

    return aiohttp.web.json_response({"status": responses})


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


# Schemas
COMMAND_SCHEMA = {
    "type": "array",
    "minItems": 2,
    "items": [
        {"type": "integer", "minimum": 0, "maximum": 255},
        {"type": "integer", "enum": ALLOWED_OPCODES},
    ],
    "additionalItems": {"type": "integer", "minimum": 0, "maximum": 255},
}

MODBUS_CMD_PAYLOAD_SCHEMA = {
    "title": "Modbus command request",
    "description": "Schema to define the modbus command (raw) request",
    "type": "object",
    "additionalProperties": False,
    "required": ["commands", "expected_response_length"],
    "properties": {
        "commands": {"type": "array", "minItems": 1, "items": COMMAND_SCHEMA},
        "expected_response_length": {"type": "integer", "minimum": 2, "maximum": 255},
        "custom_timeout": {"type": "integer", "minimum": 0},
    },
}

READ_REQ_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "reqired": ["devAddress", "regAddress", "numRegisters"],
    "properties": {
        "devAddress": {"type": "integer", "minimum": 0, "maximum": 255},
        "regAddress": {"type": "integer", "minimum": 0, "maximum": 65535},
        "numRegisters": {"type": "integer", "minimum": 1, "maximum": 127},
        "timeout": {"type": "integer", "minimum": 1},
    },
}

READ_SCHEMA = {
    "title": "Modbus Read request",
    "description": "Schema to define the modbus read request",
    "type": "object",
    "additionalProperties": False,
    "required": ["req"],
    "properties": {"req": {"type": "array", "minItems": 1, "items": READ_REQ_SCHEMA}},
}

WRITE_REQ_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "reqired": ["devAddress", "regAddress", "regValue"],
    "properties": {
        "devAddress": {"type": "integer", "minimum": 0, "maximum": 255},
        "regAddress": {"type": "integer", "minimum": 0, "maximum": 65535},
        "timeout": {"type": "integer", "minimum": 1},
        "regValue": {
            "type": ["integer", "array"],
            "minimum": 0,
            "maximum": 65535,
            "minItems": 1,
            "items": {"type": "integer", "minimum": 0, "maximum": 65535},
        },
    },
}

WRITE_SCHEMA = {
    "title": "Modbus Read request",
    "description": "Schema to define the modbus read request",
    "type": "object",
    "additionalProperties": False,
    "required": ["req"],
    "properties": {"req": {"type": "array", "minItems": 1, "items": WRITE_REQ_SCHEMA}},
}
