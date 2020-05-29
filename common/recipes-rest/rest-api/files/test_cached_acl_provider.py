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

import asyncio
import gzip
import json
import os
import signal
import tempfile
import unittest

from acl_providers import cached_acl_provider
from aiohttp.test_utils import unittest_run_loop


class TestCachedAclProvider(unittest.TestCase):
    def setUp(self):
        self.loop = asyncio.new_event_loop()
        with tempfile.NamedTemporaryFile("wb") as tmpfile:
            rules = {"deathowl": ["test", "awesome"]}
            tmpfile.write(gzip.compress(bytes(json.dumps(rules), encoding="utf-8")))
            tmpfile.seek(0)
            self.subject = cached_acl_provider.CachedAclProvider(cachepath=tmpfile.name)

    @unittest_run_loop
    async def testAclProviderExtractsRulesForIdentity(self):
        rules = await self.subject._get_permissions_for_identity("deathowl")
        self.assertEqual(["test", "awesome"], rules)

    @unittest_run_loop
    async def testAclProviderReturnsEmptyDictForNonexistentIdentity(self):
        rules = await self.subject._get_permissions_for_identity("nosuchthing")
        self.assertEqual([], rules)

    @unittest_run_loop
    async def testAclProviderGrantsPermissionIfMatching(self):
        self.assertTrue(await self.subject.is_user_authorized("deathowl", ["awesome"]))

    @unittest_run_loop
    async def testAclProviderDeniesPermissionIfNotMatching(self):
        self.assertFalse(await self.subject.is_user_authorized("deathowl", ["derp"]))

    @unittest_run_loop
    async def testAclProviderDeniesPermissionIfIdentityIsInvalid(self):
        self.assertFalse(await self.subject.is_user_authorized("nosuchthing", ["derp"]))

    @unittest_run_loop
    async def test_signal_handler(self):
        with tempfile.NamedTemporaryFile("wb") as tmpfile:
            rules = {"deathowl": ["test", "awesome"]}
            tmpfile.write(gzip.compress(bytes(json.dumps(rules), encoding="utf-8")))
            tmpfile.seek(0)
            subject = cached_acl_provider.CachedAclProvider(cachepath=tmpfile.name)
            newrules = {"deathowl": ["cats"]}
            tmpfile.seek(0)
            tmpfile.truncate()
            tmpfile.write(gzip.compress(bytes(json.dumps(newrules), encoding="utf-8")))
            tmpfile.seek(0)
            os.kill(os.getpid(), signal.SIGHUP)
            self.assertEqual(
                ["cats"], await subject._get_permissions_for_identity("deathowl")
            )
