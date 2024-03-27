#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.
#

import unittest

from common.base_interface_test import CommonInterfaceTest
from utils.cit_logger import Logger


"""
Tests eth0 v4 interface
"""


class InterfaceTest(CommonInterfaceTest, unittest.TestCase):
    def test_eth0_v4_interface(self):
        """
        Tests eth0 v4 interface
        """
        self.set_ifname("eth0")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(self.ping_v4(), 0, "Ping test for eth0 v4 failed")