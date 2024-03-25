import heapq
import json
import math
import os
import tempfile
from typing import Sequence

import aiohttp.web

DESTINATION_FILE_PATH = "/var/run/qsfp_thermal_data.json"


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

    iface_temperatures = [
        int(payload["transceiverThermalData"][iface]["temperature"])
        for iface in payload["transceiverThermalData"]
    ]

    # Format expected by fscd
    sensor_dict = {
        "timestamp": payload["timestamp"],
        "data": {
            "optics_temp_p95": {
                "value": calc_percentile(iface_temperatures, 95),
            },
        },
    }
    # Create temporary file before moving to destination to make an atomic update
    # (i.e. remove the possibility of a partial file being read)
    with tempfile.NamedTemporaryFile(
        "w", delete=False, dir=os.path.dirname(DESTINATION_FILE_PATH)
    ) as f:
        json.dump(sensor_dict, f, indent=4)

    os.rename(f.name, DESTINATION_FILE_PATH)

    return aiohttp.web.json_response({"status": "OK"})


# Utils


def calc_percentile(values: Sequence[float], percentile: int):
    """
    Calculates the percentile (e.g. P95) of values. `percentile` is an integer between
    0 and 100. The function is optimized for percentiles close to 100, although correct
    for all values.
    """
    return heapq.nlargest(math.ceil((1 - percentile / 100) * len(values)), values)[-1]


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
