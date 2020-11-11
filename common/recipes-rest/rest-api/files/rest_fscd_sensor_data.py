import aiohttp.web
import json
import tempfile
import os

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
        "th4_1": {
            "value": float,
            "ts": int,
        },
        "qsfp_1": {
            "value": float,
            "ts": int,
        },
        "qsfp_2": {
            "value": float,
            "ts": int,
        },
        "qsfp_3": {
            "value": float,
            "ts": int,
        },
        "qsfp_4": {
            "value": float,
            "ts": int,
        },
        "qsfp_5": {
            "value": float,
            "ts": int,
        },
        "qsfp_6": {
            "value": float,
            "ts": int,
        },
        "qsfp_7": {
            "value": float,
            "ts": int,
        },
        "qsfp_8": {
            "value": float,
            "ts": int,
        },
        "qsfp_9": {
            "value": float,
            "ts": int,
        },
        "qsfp_10": {
            "value": float,
            "ts": int,
        },
        "qsfp_11": {
            "value": float,
            "ts": int,
        },
        "qsfp_12": {
            "value": float,
            "ts": int,
        },
        "qsfp_13": {
            "value": float,
            "ts": int,
        },
        "qsfp_14": {
            "value": float,
            "ts": int,
        },
        "qsfp_15": {
            "value": float,
            "ts": int,
        },
        "qsfp_16": {
            "value": float,
            "ts": int,
        },
        "qsfp_17": {
            "value": float,
            "ts": int,
        },
        "qsfp_18": {
            "value": float,
            "ts": int,
        },
        "qsfp_19": {
            "value": float,
            "ts": int,
        },
        "qsfp_20": {
            "value": float,
            "ts": int,
        },
        "qsfp_21": {
            "value": float,
            "ts": int,
        },
        "qsfp_22": {
            "value": float,
            "ts": int,
        },
        "qsfp_23": {
            "value": float,
            "ts": int,
        },
        "qsfp_24": {
            "value": float,
            "ts": int,
        },
        "qsfp_25": {
            "value": float,
            "ts": int,
        },
        "qsfp_26": {
            "value": float,
            "ts": int,
        },
        "qsfp_27": {
            "value": float,
            "ts": int,
        },
        "qsfp_28": {
            "value": float,
            "ts": int,
        },
        "qsfp_29": {
            "value": float,
            "ts": int,
        },
        "qsfp_30": {
            "value": float,
            "ts": int,
        },
        "qsfp_31": {
            "value": float,
            "ts": int,
        },
        "qsfp_32": {
            "value": float,
            "ts": int,
        },
    },
}
