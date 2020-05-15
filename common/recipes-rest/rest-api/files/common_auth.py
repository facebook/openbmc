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
import typing as t

from aiohttp.web import HTTPForbidden, HTTPUnauthorized


async def auth_required(request) -> str:
    # Only expiration date is validated here,
    # since authenticity of client cert is validated on the TLS level
    if await _validate_cert_date(request):
        identity = await _extract_identity(request)
        request["identity"] = identity
        return identity
    raise HTTPUnauthorized()


async def permissions_required(request, permissions: t.List[str], context=None) -> bool:
    if not permissions:
        return True
    identity = await _extract_identity(request)
    if await request.app["acl_provider"].is_user_authorized(identity, permissions):
        return True
    else:
        raise HTTPForbidden()


async def _validate_cert_date(request) -> bool:
    peercert = request.transport.get_extra_info("peercert")
    return (
        peercert
        and datetime.datetime.strptime(peercert["notAfter"], "%b %d %H:%M:%S %Y %Z")
        > datetime.datetime.now()
    )


async def _extract_identity(request) -> str:
    peercert = request.transport.get_extra_info("peercert")
    if peercert:
        for candidate in peercert["subject"]:
            if candidate[0][0] == "commonName":
                return candidate[0][1]
    raise HTTPForbidden()
