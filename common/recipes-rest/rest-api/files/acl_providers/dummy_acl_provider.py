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

import typing as t

from . import common_acl_provider_base


class DummyAclProvider(common_acl_provider_base.AclProviderBase):
    async def _get_permissions_for_identity(self, identity: str) -> t.List[str]:
        return []

    async def is_user_authorized(self, identity: str, permissions: t.List[str]) -> bool:
        self._get_permissions_for_identity(identity)
        return True
