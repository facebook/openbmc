#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

import json

import aiohttp

import jsonschema_lite as jsonschema

from common_utils import dumps_bytestr

try:
    import pyrmd
except ImportError:
    pyrmd = None


# Handler for sensors resource endpoint
async def get_modbus_registers_raw():
    if pyrmd is None:
        raise ModuleNotFoundError()
    obj = await pyrmd.RackmonAsyncInterface.data()
    return json.dumps(obj)


async def get_modbus_registers(request: aiohttp.web.Request) -> aiohttp.web.Response:
    if pyrmd is None:
        raise ModuleNotFoundError()
    try:
        payload = await request.json()
        jsonschema.validate(payload, VALUE_DATA_REQ_SCHEMA)
    except jsonschema.ValidationError as e:
        return aiohttp.web.json_response(
            {
                "status": "Bad Request",
                "details": "Json validation error: " + str(e),
            },
            status=400,
        )
    except Exception:
        payload = {}

    response = await pyrmd.RackmonAsyncInterface.data(False, payload)
    return aiohttp.web.json_response(response, dumps=dumps_bytestr)


REGISTER_FILTER_SCHEMA = {
    "title": "Register Filter Schema",
    "description": "Schema to define a register filter",
    "type": "object",
    "additionalProperties": False,
    "properties": {
        "addressFilter": {
            "type": "array",
            "uniqueItems": True,
            "minItems": 1,
            "items": {"type": "integer", "minimum": 0, "maximum": 65535},
        },
        "nameFilter": {
            "type": "array",
            "uniqueItems": True,
            "minItems": 1,
            "items": {
                "type": "string",
            },
        },
    },
}

DEVICE_FILTER_SCHEMA = {
    "title": "Device Filter Schema",
    "description": "Schema to define a device filter",
    "type": "object",
    "additionalProperties": False,
    "minProperties": 1,
    "maxProperties": 1,
    "properties": {
        "addressFilter": {
            "type": "array",
            "uniqueItems": True,
            "minItems": 1,
            "items": {"type": "integer", "minimum": 0, "maximum": 255},
        },
        "typeFilter": {
            "type": "array",
            "uniqueItems": True,
            "minItems": 1,
            "items": {"type": "string", "enum": ["ORV2_PSU", "ORV3_PSU", "ORV3_BBU"]},
        },
    },
}

VALUE_DATA_REQ_SCHEMA = {
    "title": "Data Request",
    "description": "Schema of request",
    "type": "object",
    "properties": {
        "filter": {
            "description": "Optional filter requested monitor data",
            "type": "object",
            "additionalProperties": False,
            "properties": {
                "latestValueOnly": {"type": "boolean"},
                "deviceFilter": DEVICE_FILTER_SCHEMA,
                "registerFilter": REGISTER_FILTER_SCHEMA,
            },
        }
    },
}
