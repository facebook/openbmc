#!/usr/bin/env python3
#
# Copyright (c) 2026 Nexthop Systems Inc.
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

import json
import unittest

from common.base_eeprom_test import EepromV5Test

# Written by wedge-eeprom-config.py: lists only the EEPROMs fitted on this
# unit. chassis_eeprom is aliased to bmc_eeprom when the chassis EEPROM is
# not populated, so both would then point at the same sysfs node.
WEUTIL_CONFIG = "/etc/weutil/eeprom.json"


def _eeprom_sysfs_paths():
    try:
        with open(WEUTIL_CONFIG) as fp:
            devices = json.load(fp)["eeprom devices"]
    except Exception:
        return {}
    return {d["name"]: d["sysfs_path"] for d in devices}


def _skip_unless_dedicated_eeprom(name):
    paths = _eeprom_sysfs_paths()
    if name not in paths:
        raise unittest.SkipTest(f"{name} is not configured in {WEUTIL_CONFIG}")
    aliases = [n for n, p in paths.items() if p == paths[name] and n != name]
    if aliases:
        raise unittest.SkipTest(f"{name} is aliased to {aliases[0]}, not fitted")


class ChassisEepromTest(EepromV5Test, unittest.TestCase):
    """
    Test for CHASSIS EEPROM (on the SMB, holds the BMC and switch ASIC MACs).
    """

    @classmethod
    def setUpClass(cls):
        _skip_unless_dedicated_eeprom("chassis_eeprom")

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e chassis_eeprom"]

    def set_product_name(self):
        self.product_name = ["M4062NHP"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SMB"]

    # The x86 MAC lives in the SCM EEPROM.
    def test_x86_mac(self):
        pass


class BmcEepromTest(EepromV5Test, unittest.TestCase):
    """
    Test for BMC EEPROM (holds only the BMC MAC).
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e bmc_eeprom"]

    def set_product_name(self):
        self.product_name = ["BMC"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["BMC"]

    def test_x86_mac(self):
        pass

    def test_switch_asic_mac(self):
        pass


class ScmEepromTest(EepromV5Test, unittest.TestCase):
    """
    Test for SCM EEPROM (holds only the x86 CPU MAC).
    """

    @classmethod
    def setUpClass(cls):
        _skip_unless_dedicated_eeprom("scm_eeprom")

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e scm_eeprom"]

    def set_product_name(self):
        self.product_name = ["SCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SCM"]

    def test_bmc_mac(self):
        pass

    def test_switch_asic_mac(self):
        pass
