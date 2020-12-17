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

# from request.transport.get_extra_info("peercert")
EXAMPLE_USER_CERT = {
    "notAfter": "Jan  2 00:00:00 2000 GMT",
    "notBefore": "Jan  1 00:00:00 2000 GMT",
    "subject": ((("commonName", "user:a_username/a_hostname.example.com"),),),
}


EXAMPLE_HOST_CERT = {
    "notAfter": "Jan  2 00:00:00 2000 GMT",
    "notBefore": "Jan  1 00:00:00 2000 GMT",
    "subject": ((("commonName", "host:no_one/a_hostname.example.com"),),),
}

import datetime
import ipaddress
from unittest import mock

import acl_providers.cached_acl_provider
import common_auth
from aiohttp import web
from aiohttp.test_utils import AioHTTPTestCase, make_mocked_request


class TestCommonAuth(AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        self.patches = [
            # Freeze time to reliably test cert dates
            mock.patch("time.time", autospec=True, return_value=0),
            mock.patch(
                "common_auth.now",
                return_value=datetime.datetime.fromtimestamp(0),
            ),
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    def test_extract_identity_from_peercert_user_identity(self):
        req = self.make_mocked_request_user_cert()
        identity = common_auth._extract_identity(req)

        self.assertEqual(identity, common_auth.Identity(user="a_username", host=None))

    def test_extract_identity_from_peercert_host_identity(self):
        req = self.make_mocked_request_host_cert()
        identity = common_auth._extract_identity(req)

        self.assertEqual(
            identity, common_auth.Identity(user=None, host="a_hostname.example.com")
        )

    def test_extract_identity_ipv6(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            wraps={
                "peername": ("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 45654, 0, 0)
            }.get
        )
        res = common_auth._extract_identity(req)
        self.assertEqual(
            res,
            common_auth.Identity(
                user=None, host=ipaddress.IPv6Address("2001:db8:85a3::8a2e:370:7334")
            ),
        )

    def test_extract_identity_ipv4(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            wraps={"peername": ("192.168.1.1", 45654)}.get
        )
        res = common_auth._extract_identity(req)
        self.assertEqual(
            res,
            common_auth.Identity(user=None, host=ipaddress.IPv4Address("192.168.1.1")),
        )

    def test_extract_identity_from_peercert_no_identity(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            return_value={
                **EXAMPLE_USER_CERT,
                "subject": ((("commonName", "badlyformedcommonname"),),),
            }
        )

        res = common_auth._extract_identity(req)
        self.assertEqual(res, common_auth.NO_IDENTITY)

    def test_extract_identity_from_peercert_empty_user(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            return_value={
                **EXAMPLE_HOST_CERT,
                "subject": ((("commonName", "host:/a_hostname.example.com"),),),
            }
        )

        res = common_auth._extract_identity(req)
        self.assertEqual(
            res, common_auth.Identity(user=None, host="a_hostname.example.com")
        )

    def test_extract_identity_from_peercert_empty_host(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            return_value={
                **EXAMPLE_HOST_CERT,
                "subject": ((("commonName", "user:a_username/"),),),
            }
        )

        res = common_auth._extract_identity(req)
        self.assertEqual(res, common_auth.Identity(user="a_username", host=None))

    def test_extract_identity_from_peercert_no_subject_field(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            return_value={**EXAMPLE_USER_CERT, "subject": ()}
        )

        res = common_auth._extract_identity(req)
        self.assertEqual(res, common_auth.NO_IDENTITY)

    def test_extract_identity_from_peercert_cert_expired(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            return_value={**EXAMPLE_USER_CERT, "notAfter": "Jan  1 00:00:00 1969 GMT"}
        )

        with self.assertRaises(ValueError) as cm:
            common_auth._extract_identity_from_peercert(req)

        self.assertEqual(
            str(cm.exception), "Peer certificate's date is invalid/expired"
        )

    def test_extract_identity_no_identity(self):
        req = make_mocked_request("GET", "/")
        res = common_auth._extract_identity(req)

        self.assertEqual(res, common_auth.NO_IDENTITY)

    def test_extract_identity_peercert(self):
        req = self.make_mocked_request_user_cert()
        res = common_auth._extract_identity(req)

        self.assertEqual(res, common_auth.Identity(user="a_username", host=None))

    def test_validate_cert_date(self):
        req = self.make_mocked_request_user_cert()
        self.assertEqual(common_auth._validate_cert_date(req), True)

    def test_validate_cert_no_cert(self):
        req = make_mocked_request("GET", "/")

        with self.assertLogs(level="INFO") as cm:
            self.assertEqual(common_auth._validate_cert_date(req), False)

        self.assertEqual(
            cm.records[0].msg,
            "AUTH:Client did not send client certificates for request [GET]/",
        )

    def test_validate_cert_expired(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            return_value={**EXAMPLE_USER_CERT, "notAfter": "Jan  1 00:00:00 1969 GMT"}
        )

        with self.assertLogs(level="INFO") as cm:
            self.assertEqual(common_auth._validate_cert_date(req), False)

        self.assertEqual(
            cm.records[0].msg,
            "AUTH:Client sent client certificates for request [GET]/, but it expired at: Jan  1 00:00:00 1969 GMT",  # noqa: B950
        )

    def test_permissions_required_allowed(self):
        req = mock.Mock(spec=web.Request)
        req.app = {
            "acl_provider": mock.create_autospec(
                spec=acl_providers.cached_acl_provider.CachedAclProvider
            )
        }
        req.app["acl_provider"].is_user_authorized.return_value = True

        with mock.patch(
            "common_auth._extract_identity", autospec=True, return_value="<user>"
        ):
            res = common_auth.permissions_required(
                request=req, permissions=[], context=None
            )

        self.assertEqual(res, True)

    def test_permissions_required_denied(self):
        req = mock.Mock(spec_set=web.Request)
        req.method = "GET"
        req.path = "/"
        req.app = {
            "acl_provider": mock.create_autospec(
                spec=acl_providers.cached_acl_provider.CachedAclProvider
            )
        }
        req.app["acl_provider"].is_authorized.return_value = False

        with mock.patch(
            "common_auth._extract_identity", autospec=True, return_value="<user>"
        ), self.assertLogs(level="INFO") as logs, self.assertRaises(web.HTTPForbidden):
            common_auth.permissions_required(
                request=req, permissions=["<perm>"], context=None
            )

        self.assertEqual(
            logs.records[0].msg,
            "AUTH:Failed to authorize <user> for endpoint [GET]/ . Required permissions :['<perm>']",  # noqa: B950
        )

    # Utils
    def make_mocked_request_user_cert(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            wraps={"peercert": EXAMPLE_USER_CERT}.get
        )
        return req

    def make_mocked_request_host_cert(self):
        req = make_mocked_request("GET", "/")
        req.transport.get_extra_info = mock.Mock(
            wraps={"peercert": EXAMPLE_HOST_CERT}.get
        )
        return req

    async def get_application(self):
        # This method is required by AioHTTPTestCase but not used yet
        return web.Application()
