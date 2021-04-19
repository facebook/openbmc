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

import abc
import ipaddress
import re
import socket
import typing as t

from common_auth import Identity


LINK_LOCAL_IPV6 = ipaddress.IPv6Address("fe80::2")

# e.g. "a_hostname-oob.example.com" -> prefix="a_hostname"
RE_OOB_SUFFIX = re.compile(r"(?P<prefix>^[^.]+?)(?P<suffix>-oob)?(?=[.])")


class HostRoles:
    # Role that matches any managed host
    MANAGED_HOST_ANY = "MANAGED_HOST_ANY"


class AclProviderBase(metaclass=abc.ABCMeta):
    def __init__(self):
        super().__init__()

    def is_authorized(self, identity: Identity, permissions: t.List[str]) -> bool:
        """
        Returns True if any user/host role matching the identity are in the given
        permissions list
        """
        user_authorized = self.is_user_authorized(identity, permissions)
        host_authorized = self.is_host_authorized(identity, permissions)

        return user_authorized or host_authorized

    @abc.abstractmethod
    def is_user_authorized(self, identity: Identity, permissions: t.List[str]) -> bool:
        """
        Returns True if any role matching the user identity are in the given
        permissions list
        """
        pass

    @classmethod
    def is_host_authorized(cls, identity: Identity, permissions: t.List[str]) -> bool:
        """
        Returns True if any role matching the host identity are in the given
        permissions list.
        """
        host_roles = cls._get_roles_for_host(identity)
        return any(role in permissions for role in host_roles)

    ## Utils
    @classmethod
    def _get_roles_for_host(cls, identity: Identity) -> t.List[str]:
        if identity.host and cls._is_managed_host(identity):
            return [HostRoles.MANAGED_HOST_ANY]

        return []

    @classmethod
    def _is_managed_host(cls, identity: Identity) -> bool:
        # If the identity is from a link-local address, assume it's one of
        # the managed hosts
        if identity.host == LINK_LOCAL_IPV6:
            return True

        # Check if the host identity from the peer certificate is a known
        # managed hostname
        return (
            type(identity.host) == str and identity.host in cls._get_managed_hostnames()
        )

    @staticmethod
    def _get_managed_hostnames() -> t.List[str]:
        # TODO: Replace de-oobifying with a better source of truth for managed
        # hostnames
        oob_hostname = socket.gethostname()

        # e.g. "blah-oob.example.com" -> "blah.example.com"
        hostname = RE_OOB_SUFFIX.sub(r"\g<prefix>", oob_hostname)

        if oob_hostname != hostname:
            return [hostname]

        return []
