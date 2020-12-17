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

import datetime
import re
import typing as t

from aiohttp.log import server_logger
from aiohttp.web import Request, HTTPForbidden, HTTPUnauthorized


# Certificate commonName regex
# e.g. "host:root/example.com" -> type="host", user="root", host="example.com"
RE_CERT_COMMON_NAME = re.compile(r"^(?P<type>[^:]+):(?P<user>[^/]+)/(?P<host>[^/]+)$")


async def auth_required(request) -> str:
    # Only expiration date is validated here,
    # since authenticity of client cert is validated on the TLS level
    if await _validate_cert_date(request):
        identity = await _extract_identity(request)
        request["identity"] = identity
        request.headers["identity"] = identity
        return identity
    raise HTTPUnauthorized()


async def permissions_required(request, permissions: t.List[str], context=None) -> bool:
    identity = await _extract_identity(request)
    if await request.app["acl_provider"].is_user_authorized(identity, permissions):
        return True
    else:
        server_logger.info(
            "AUTH:Failed to authorize %s for endpoint [%s]%s . Required permissions :%s"
            % (identity, request.method, request.path, str(permissions))
        )
        raise HTTPForbidden()


async def _validate_cert_date(request) -> bool:
    peercert = request.transport.get_extra_info("peercert")
    if not peercert:
        server_logger.info(
            "AUTH:Client did not send client certificates for request [%s]%s"
            % (request.method, request.path)
        )
        return False
    cert_valid = (
        datetime.datetime.strptime(peercert["notAfter"], "%b %d %H:%M:%S %Y %Z") > now()
    )
    if not cert_valid:
        server_logger.info(
            "AUTH:Client sent client certificates for request [%s]%s, but it expired at: %s"
            % (request.method, request.path, peercert["notAfter"])
        )
        return False
    return True


async def _extract_identity(request: Request) -> str:
    peercert = request.transport.get_extra_info("peercert")
    if not peercert or not peercert.get("subject"):
        return ""

    for key, value in peercert["subject"][0]:
        if key == "commonName":
            m = RE_CERT_COMMON_NAME.match(value)

            if m and m.group("type") == "user":
                return m.group("user")
    return ""


def now() -> datetime.datetime:
    # Just a simple wrapper so it's easier to mock (as datetime.datetime.now
    # is a built-in)
    return datetime.datetime.now()
