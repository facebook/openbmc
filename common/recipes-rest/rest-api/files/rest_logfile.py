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

import os

from aiohttp import web
from common_utils import async_in_common_executor, dumps_bytestr


LOGFILE = "/mnt/data/logfile"

# rsyslog rotates logfile at 200KB into a single .0 generation, so an event
# from an hour ago may already have moved. Callers after a complete history
# have to ask for both.
ROTATED_LOGFILE = LOGFILE + ".0"

DEFAULT_LINES = 200
MAX_LINES = 200


def _read_lines(path):
    if not os.path.exists(path):
        return []
    with open(path, "r", errors="replace") as f:
        return f.read().splitlines()


def get_logfile(lines=DEFAULT_LINES, include_rotated=False):
    entries = []
    sources = []

    if include_rotated:
        rotated = _read_lines(ROTATED_LOGFILE)
        if rotated:
            entries += rotated
            sources.append(ROTATED_LOGFILE)

    current = _read_lines(LOGFILE)
    if current:
        entries += current
        sources.append(LOGFILE)

    truncated = len(entries) > lines
    if truncated:
        entries = entries[-lines:]

    result = {
        "Information": {
            # File order, not time order. Lines written before NTP sync carry
            # the image's build-default date, so entries are not monotonic and
            # must not be sorted or max()'d by timestamp.
            "entries": entries,
            "count": len(entries),
            "truncated": truncated,
            "sources": sources,
        },
        "Actions": [],
        "Resources": [],
    }
    return result


def _bad_request(reason):
    return web.json_response(
        {"Information": {"reason": reason}, "Actions": [], "Resources": []},
        dumps=dumps_bytestr,
        status=400,
    )


async def post_logfile(request: web.Request) -> web.Response:
    # An empty body is a valid request for the defaults.
    if not request.can_read_body:
        payload = {}
    else:
        try:
            payload = await request.json()
        except Exception:
            return _bad_request("body must be JSON")
    if not isinstance(payload, dict):
        return _bad_request("body must be a JSON object")

    lines = payload.get("lines", DEFAULT_LINES)
    # bool is a subclass of int, so reject it explicitly.
    if isinstance(lines, bool) or not isinstance(lines, int):
        return _bad_request("lines must be an integer")
    lines = max(1, min(lines, MAX_LINES))

    include_rotated = payload.get("include_rotated", False)
    if not isinstance(include_rotated, bool):
        return _bad_request("include_rotated must be a boolean")

    # Reading the file blocks, so keep it off the event loop. Positional args:
    # run_in_executor does not forward keywords.
    result = await async_in_common_executor(get_logfile)(lines, include_rotated)
    return web.json_response(result, dumps=dumps_bytestr, status=200)
