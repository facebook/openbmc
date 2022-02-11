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
import ipaddress
import socket
import unittest
from unittest import mock

import common_auth
from acl_providers.common_acl_provider_base import AclProviderBase

EXAMPLE_OOB_HOSTNAME = "a_hostname-oob.example.com"
EXAMPLE_MANAGED_HOSTNAME = "a_hostname.example.com"

EXAMPLE_MANAGED_HOST_IDENTITY = common_auth.Identity(
    user=None, host=EXAMPLE_MANAGED_HOSTNAME
)


class MockAclProvider(AclProviderBase):
    always_authorize_user = False
    always_authorize_host = False

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.is_user_authorized = mock.Mock(wraps=self.is_user_authorized)
        self.is_host_authorized = mock.Mock(wraps=self.is_host_authorized)

    def is_user_authorized(self, identity, permissions):
        return self.always_authorize_user

    def is_host_authorized(self, identity, permissions):
        return self.always_authorize_host


class TestCachedAclProvider(unittest.TestCase):
    def setUp(self):
        self.patches = [
            mock.patch(
                "socket.gethostname",
                autospec=True,
                return_value=EXAMPLE_OOB_HOSTNAME,
            )
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    def test_is_host_authorized_no_identity(self):
        res = AclProviderBase.is_host_authorized(common_auth.NO_IDENTITY, [])
        self.assertEqual(res, False)

    def test_is_host_authorized_no_permissions(self):
        res = AclProviderBase.is_host_authorized(EXAMPLE_MANAGED_HOST_IDENTITY, [])
        self.assertEqual(res, False)

    def test_is_host_authorized(self):
        res = AclProviderBase.is_host_authorized(
            EXAMPLE_MANAGED_HOST_IDENTITY, ["MANAGED_HOST_ANY"]
        )
        self.assertEqual(res, True)

    def test_get_roles_for_host_no_identity(self):
        host_roles = AclProviderBase._get_roles_for_host(common_auth.NO_IDENTITY)
        self.assertEqual(host_roles, [])

    def test_get_roles_for_host_managed_host(self):
        host_roles = AclProviderBase._get_roles_for_host(EXAMPLE_MANAGED_HOST_IDENTITY)
        self.assertEqual(host_roles, ["MANAGED_HOST_ANY"])

    def test_get_roles_for_host_not_managed_host(self):
        identity = common_auth.Identity(user=None, host="another_hostname.example.com")
        host_roles = AclProviderBase._get_roles_for_host(identity)
        self.assertEqual(host_roles, [])

    def test_is_managed_host_no_identity(self):
        res = AclProviderBase._is_managed_host(common_auth.NO_IDENTITY)
        self.assertEqual(res, False)

    def test_is_managed_host_link_local(self):
        res = AclProviderBase._is_managed_host(
            common_auth.Identity(user=None, host=ipaddress.IPv6Address("fe80::2"))
        )
        self.assertEqual(res, True)

    def test_is_managed_host_managed_hostname(self):
        res = AclProviderBase._is_managed_host(
            common_auth.Identity(user=None, host=EXAMPLE_MANAGED_HOSTNAME)
        )
        self.assertEqual(res, True)

    def test_get_managed_hostnames(self):
        socket.gethostname.return_value = EXAMPLE_OOB_HOSTNAME
        managed_hostnames = AclProviderBase._get_managed_hostnames()
        self.assertEqual(managed_hostnames, [EXAMPLE_MANAGED_HOSTNAME])

    def test_get_managed_hostnames_non_oob(self):
        socket.gethostname.return_value = "a_hostname.example.com"
        managed_hostnames = AclProviderBase._get_managed_hostnames()
        # Should not return a hostname if the BMC is not configured with a -oob
        # hostname
        self.assertEqual(managed_hostnames, [])

    def test_is_authorized_user(self):
        acl_provider = MockAclProvider()
        acl_provider.always_authorize_user = True

        self.assertEqual(acl_provider.is_authorized(common_auth.NO_IDENTITY, []), True)

    def test_is_authorized_host(self):
        acl_provider = MockAclProvider()
        acl_provider.always_authorize_host = True

        self.assertEqual(acl_provider.is_authorized(common_auth.NO_IDENTITY, []), True)

    def test_is_authorized_unauthorized(self):
        acl_provider = MockAclProvider()

        self.assertEqual(acl_provider.is_authorized(common_auth.NO_IDENTITY, []), False)
