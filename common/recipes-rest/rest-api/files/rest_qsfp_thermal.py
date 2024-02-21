import json
import os
import tempfile

import aiohttp.web

DESTINATION_FILE_PATH = "/tmp/qsfp_thermal_data.json"


async def post_qsfp_thermal_data(request: aiohttp.web.Request) -> aiohttp.web.Response:
    """
    Endpoint for collecting optics thermal data from qsfp_service.
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


# Utils
def _validate_payload(payload, schema, path="") -> None:
    if not _schema_match(payload, schema):
        raise ValueError(
            "Schema mismatch in {path}: expected value ({x}) to match schema {y}".format(  # noqa: B950
                path=path or ".", x=repr(payload), y=repr(schema)
            )
        )

    if isinstance(schema, dict):
        for key, value in payload.items():
            _validate_payload(
                payload=value,
                schema=schema[key] if key in schema else schema[""],
                path=path + "." + key,
            )


def _schema_match(payload, schema) -> bool:
    if isinstance(payload, dict) and isinstance(schema, dict):
        return schema.keys() == payload.keys() or schema.keys() == {""}

    elif isinstance(schema, type):
        return isinstance(payload, schema)

    elif isinstance(schema, str):
        return payload == schema

    return False


# HACK: Temporary schema until we use something more robust
PAYLOAD_SCHEMA = {
    "version": str,
    "timestamp": int,
    "dataCenter": str,
    "hostnameScheme": str,
    "transceiverThermalData": {
        "": {
            "moduleMediaInterface": str,
            "temperature": str,
        },
    },
}
