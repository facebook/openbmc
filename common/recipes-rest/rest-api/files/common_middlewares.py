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
from contextlib import suppress

import common_auth
from acl_config import RULES
from aiohttp.log import server_logger
from aiohttp.web_exceptions import HTTPException, HTTPForbidden, HTTPInternalServerError


async def jsonerrorhandler(app, handler):
    async def middleware_handler(request):
        try:
            resp = await handler(request)
        except HTTPException as exc:
            resp = exc
        except Exception as exc:
            body = {
                "Information": {"reason": repr(exc)},
                "Actions": [],
                "Resources": [],
            }

            resp = HTTPInternalServerError(
                body=json.dumps(body), content_type="application/json"
            )
            server_logger.exception("Error handling request", exc_info=exc)
        return resp

    return middleware_handler


async def auth_enforcer(app, handler):
    async def middleware_handler(request):
        acls = []
        with suppress(KeyError):
            acls = RULES[request.path][request.method]
        # We only allow GET endpoints without authorization.
        # Anything else will be forbidden.
        if not acls and request.method != "GET":
            server_logger.info(
                "AUTH:Missing acl config for non-get[%s] endpoint %s. Blocking access"
                % (request.method, request.path)
            )
            raise HTTPForbidden()
        if acls:
            identity = await common_auth.auth_required(request)
            await common_auth.permissions_required(request, acls)
            server_logger.info(
                "AUTH:Authorized %s for [%s]%s"
                % (identity, request.method, request.path)
            )
        resp = await handler(request)
        return resp

    return middleware_handler
