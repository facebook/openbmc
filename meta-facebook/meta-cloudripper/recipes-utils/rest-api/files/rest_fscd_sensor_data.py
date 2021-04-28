import json
import os
import tempfile

import aiohttp.web

from rest_fscd_sensor_payload import PAYLOAD_SCHEMA

DESTINATION_FILE_PATH = "/tmp/incoming_fscd_sensor_data.json"


async def post_fscd_sensor_data(request: aiohttp.web.Request) -> aiohttp.web.Response:
    """
    Endpoint for collecting data from the managed host and saving it to
    DESTINATION_FILE_PATH. Request payload MUST match PAYLOAD_SCHEMA.
    """
    try:
        payload = await request.json()
        _validate_payload(payload=payload, schema=PAYLOAD_SCHEMA)

    except ValueError as e:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Invalid JSON payload: " + str(e),
            },
            status=400,
        )

    # Create temporary file before moving to destination to make an atomic update
    # (i.e. remove the possibility of a partial file being read)
    with tempfile.NamedTemporaryFile("w", delete=False) as f:
        json.dump(payload, f)

    os.rename(f.name, DESTINATION_FILE_PATH)

    return aiohttp.web.json_response({"status": "OK"})


## Utils
def _validate_payload(payload, schema, path="") -> bool:
    if not _schema_match(payload, schema):
        raise ValueError(
            "Schema mismatch in {path}: expected value ({x}) to match schema {y}".format(  # noqa: B950
                path=path or ".", x=repr(payload), y=repr(schema)
            )
        )

    if type(schema) == dict:
        for key, value in payload.items():
            _validate_payload(payload=value, schema=schema[key], path=path + "." + key)


def _schema_match(payload, schema):
    if type(payload) == type(schema) == dict:
        return schema.keys() == payload.keys()

    elif type(schema) == type:
        return type(payload) == schema

    elif type(schema) == str:
        return payload == schema

    return False

