import unittest

from common.base_eeprom_test import CommonEepromTest
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class ChassisEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for weutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e CHASSIS"]

    def set_product_name(self):
        self.product_name = ["MINIPACK3"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FCM"]


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class SCMEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for seutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e SCM"]

    def set_product_name(self):
        self.product_name = ["MINIPACK3_SCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SCM"]
