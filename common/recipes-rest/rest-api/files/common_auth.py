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
import ipaddress
import re
import typing as t
from contextlib import suppress

from aiohttp.log import server_logger
from aiohttp.web import HTTPForbidden, Request

Identity = t.NamedTuple(
    "Identity",
    [
        # A user as identified by a TLS certificate
        ("user", t.Optional[str]),
        # The hostname string as identified by a TLS certificate OR the
        # source ip address
        (
            "host",
            t.Union[None, str, ipaddress.IPv6Address, ipaddress.IPv4Address],
        ),
    ],
)


NO_IDENTITY = Identity(user=None, host=None)


# Certificate commonName regex
# e.g. "host:root/example.com" -> type="host", user="root", host="example.com"
# FB certs have host/user/svc magic in their CN, unlike non proprietary certs
# This regex identifies those certs
RE_SPECIAL_CERT_COMMON_NAME = re.compile(
    r"^(?P<type>[^:]+):(?P<user_or_svc>[^/]*)(/(?P<host>[^/]*))?$"
)

RE_IPV6_LINK_LOCAL_SUFFIX = re.compile("%[a-z0-9]+$")


def auth_required(request) -> Identity:
    identity = _extract_identity(request)
    request["identity"] = identity
    return identity


def permissions_required(
    request: Request, permissions: t.List[str], context=None
) -> bool:
    identity = _extract_identity(request)
    if request.app["acl_provider"].is_authorized(identity, permissions):
        return True
    else:
        server_logger.info(
            (
                "AUTH:Failed to authorize %s for endpoint [%s]%s ."
                " Required permissions :%s"
            )
            % (identity, request.method, request.path, str(permissions))
        )
        raise HTTPForbidden()


def _validate_cert_date(request) -> bool:
    # Only expiration date is validated here,
    # since authenticity of client cert is validated on the TLS level
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
            (
                "AUTH:Client sent client certificates for request [%s]%s,"
                " but it expired at: %s"
            )
            % (request.method, request.path, peercert["notAfter"])
        )
        return False
    return True


def _extract_identity(request: Request) -> Identity:
    # Try extracting identity from TLS peer certificate
    with suppress(ValueError):
        return _extract_identity_from_peercert(request)

    # If there's no cert identity, try setting the host identity as the
    # peer ip address
    with suppress(ValueError, IndexError, KeyError, TypeError):
        assert request.transport is not None
        addr = request.transport.get_extra_info("peername")[0]
        addr = RE_IPV6_LINK_LOCAL_SUFFIX.sub("", addr)

        return Identity(user=None, host=ipaddress.ip_address(addr))

    return NO_IDENTITY


def _extract_identity_from_peercert(request: Request) -> Identity:
    assert request.transport is not None
    peercert = request.transport.get_extra_info("peercert")
    if not peercert or not peercert.get("subject"):
        raise ValueError("No identity found in request")

    if not _validate_cert_date(request):
        raise ValueError("Peer certificate's date is invalid/expired")

    for key, value in peercert["subject"][0]:
        if key == "commonName":
            m = RE_SPECIAL_CERT_COMMON_NAME.match(value)

            if m and m.group("type") in ["user", "svc"]:
                return Identity(
                    user=m.group("type") + ":" + m.group("user_or_svc"),
                    host=None,
                )

            if m and m.group("type") == "host":
                return Identity(user=None, host=m.group("host"))

    raise ValueError("No identity found in request")


def now() -> datetime.datetime:
    # Just a simple wrapper so it's easier to mock (as datetime.datetime.now
    # is a built-in)
    return datetime.datetime.now()
