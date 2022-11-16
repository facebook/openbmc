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
import functools
import json
import re
from contextlib import suppress
from typing import List, Optional

import acl_config
import common_auth
from aiohttp.log import server_logger
from aiohttp.web import Request
from aiohttp.web_exceptions import (
    HTTPException,
    HTTPForbidden,
    HTTPInternalServerError,
    HTTPTooManyRequests,
)

LOCALHOST_ADDRS = {
    "::1",
    "127.0.0.1",
}


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


async def auth_enforcer(app, handler):  # noqa: C901
    class RuleRegexp:
        def __init__(self, path_regexp: str, acls: List[str]):
            self.path_regexp = re.compile(path_regexp)
            self.acls = acls

    class ACLs:
        def __init__(self, rules_plain, rules_regexp_raw):
            self.rules_plain = rules_plain
            rules_regexp = {}
            for path_regexp in rules_regexp_raw:
                acls = rules_regexp_raw[path_regexp]
                for method in acls:
                    if method not in rules_regexp:
                        rules_regexp[method] = []
                    rules_regexp[method].append(RuleRegexp(path_regexp, acls[method]))
            self.rules_regexp = rules_regexp

        @functools.lru_cache(maxsize=1024)
        def get(self, method: str, path: str) -> Optional[List[str]]:
            with suppress(KeyError):
                return self.rules_plain[path][method]
            with suppress(KeyError):
                for rule in self.rules_regexp[method]:
                    if rule.path_regexp.match(path):
                        return rule.acls
            return None

    acls_handler = ACLs(acl_config.RULES, acl_config.RULES_REGEXP)

    async def middleware_handler(request):
        # Assume all requests originating from localhost are privileged
        if _is_request_from_localhost(request):
            resp = await handler(request)
            return resp

        acls = acls_handler.get(request.method, request.path)

        # We only allow GET endpoints without authorization.
        # Anything else will be forbidden.
        if acls is None and request.method != "GET":
            server_logger.info(
                (
                    "AUTH:Missing acl config for non-get[%s] endpoint %s."
                    " Blocking access"
                )
                % (request.method, request.path)
            )
            raise HTTPForbidden()
        identity = None
        if acls is not None:
            identity = common_auth.auth_required(request)  # type: common_auth.Identity
            common_auth.permissions_required(request, acls)
            server_logger.info(
                "AUTH:Authorized %s for [%s]%s"
                % (identity, request.method, request.path)
            )
        resp = await handler(request)
        if identity:
            resp.headers["identity"] = str(identity)
        return resp

    return middleware_handler


def _is_request_from_localhost(request: Request) -> bool:
    assert request.transport is not None
    peer_ip = request.transport.get_extra_info("peername")[0]
    return peer_ip in LOCALHOST_ADDRS


async def ratelimiter(app, handler):
    # get ratelimiter from app instance
    ratelimiter = app["ratelimiter"]

    # Rate limit per useragent, request.path, request.method
    async def middleware_handler(request):
        user_agent = request.headers.get("User-Agent")
        if await ratelimiter.is_limited(request.path, request.method, user_agent):
            raise HTTPTooManyRequests()
        else:
            resp = await handler(request)
            return resp

    return middleware_handler
