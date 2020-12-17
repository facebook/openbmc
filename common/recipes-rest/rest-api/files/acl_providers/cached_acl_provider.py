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

import gzip
import json
import signal
import typing as t
from contextlib import suppress
from types import FrameType
from typing import Optional

from aiohttp.log import server_logger

from . import common_acl_provider_base


def load_acl_cache(cpath: str) -> t.Dict[str, t.List[str]]:
    with gzip.open(cpath, "rt", encoding="utf-8") as compressed:
        aclcache = json.loads(compressed.read())
        server_logger.info("Loaded acl cache with %d entries" % len(aclcache))
        return aclcache


class CachedAclProvider(common_acl_provider_base.AclProviderBase):
    def __init__(self, cachepath: Optional[str] = None):
        self.cachepath = cachepath
        self.aclrules = {}
        self._load_aclrules()

        self.oldhandler = signal.getsignal(signal.SIGHUP)
        signal.signal(signal.SIGHUP, self.signal_handler)

    def __del__(self):
        if getattr(self, "oldhandler", None):
            signal.signal(signal.SIGHUP, self.oldhandler)

    def signal_handler(self, sig: int, frame: FrameType):
        self._load_aclrules()

    async def _get_permissions_for_identity(self, identity: str) -> t.List[str]:
        with suppress(KeyError):
            return self.aclrules[identity]
        return []

    async def is_user_authorized(self, identity: str, permissions: t.List[str]) -> bool:
        roles = await self._get_permissions_for_identity(identity)
        return any(value for value in roles if value in permissions)

    def _load_aclrules(self) -> None:
        if not self.cachepath:
            server_logger.warning(
                "{self}: Cache path ({cachepath}) is not defined, not loading ACL".format(  # noqa: B950
                    self=self, cachepath=repr(self.cachepath)
                )
            )
            return

        self.aclrules = load_acl_cache(self.cachepath)
