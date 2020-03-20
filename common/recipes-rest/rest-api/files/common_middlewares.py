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

from aiohttp.log import server_logger
from aiohttp.web_exceptions import HTTPException, HTTPInternalServerError


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
