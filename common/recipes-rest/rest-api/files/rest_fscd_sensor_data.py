import json
import os
import tempfile

import aiohttp.web

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


# HACK: Temporary schema until we use something more robust
PAYLOAD_SCHEMA = {
    "global_ts": int,
    "data": {
        "th4_1_temp": {  # Tomahawk 4 core temperature
            "value": float,
            "ts": int,
        },
        "qsfp_1_temp": {  # QSFP port 1 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_2_temp": {  # QSFP port 2 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_3_temp": {  # QSFP port 3 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_4_temp": {  # QSFP port 4 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_5_temp": {  # QSFP port 5 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_6_temp": {  # QSFP port 6 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_7_temp": {  # QSFP port 7 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_8_temp": {  # QSFP port 8 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_9_temp": {  # QSFP port 9 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_10_temp": {  # QSFP port 10 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_11_temp": {  # QSFP port 11 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_12_temp": {  # QSFP port 12 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_13_temp": {  # QSFP port 13 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_14_temp": {  # QSFP port 14 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_15_temp": {  # QSFP port 15 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_16_temp": {  # QSFP port 16 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_17_temp": {  # QSFP port 17 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_18_temp": {  # QSFP port 18 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_19_temp": {  # QSFP port 19 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_20_temp": {  # QSFP port 20 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_21_temp": {  # QSFP port 21 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_22_temp": {  # QSFP port 22 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_23_temp": {  # QSFP port 23 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_24_temp": {  # QSFP port 24 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_25_temp": {  # QSFP port 25 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_26_temp": {  # QSFP port 26 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_27_temp": {  # QSFP port 27 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_28_temp": {  # QSFP port 28 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_29_temp": {  # QSFP port 29 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_30_temp": {  # QSFP port 30 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_31_temp": {  # QSFP port 31 temperature
            "value": float,
            "ts": int,
        },
        "qsfp_32_temp": {  # QSFP port 32 temperature
            "value": float,
            "ts": int,
        },
    },
}
