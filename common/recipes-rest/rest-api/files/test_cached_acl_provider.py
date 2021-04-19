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

import common_auth
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

    def testAclProviderExtractsRulesForUserIdentity(self):
        identity = common_auth.Identity(user="deathowl", host=None)
        rules = self.subject._get_permissions_for_user_identity(identity)
        self.assertEqual(["test", "awesome"], rules)

    def testAclProviderReturnsEmptyDictForNonexistentUserIdentity(self):
        rules = self.subject._get_permissions_for_user_identity(
            common_auth.Identity(user="nosuchthing", host=None)
        )
        self.assertEqual([], rules)

    def testAclProviderGrantsPermissionIfMatching(self):
        identity = common_auth.Identity(user="deathowl", host=None)
        self.assertTrue(self.subject.is_user_authorized(identity, ["awesome"]))

    def testAclProviderDeniesPermissionIfNotMatching(self):
        identity = common_auth.Identity(user="deathowl", host=None)
        self.assertFalse(self.subject.is_user_authorized(identity, ["derp"]))

    def testAclProviderDeniesPermissionIfIdentityIsInvalid(self):
        identity = common_auth.Identity(user="nosuchthing", host=None)
        self.assertFalse(self.subject.is_user_authorized(identity, ["derp"]))

    def test_signal_handler(self):
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
                ["cats"],
                subject._get_permissions_for_user_identity(
                    common_auth.Identity(user="deathowl", host=None)
                ),
            )
