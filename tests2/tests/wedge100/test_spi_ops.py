#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
import os
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


class SpiOpsTest(unittest.TestCase):
    def setUp(self):
        self.spi_util = "/usr/local/bin/spi_util.sh"
        self.tmp_bios = "/tmp/._cit_bios.bin"
        self.tmp_eeprom = "/tmp/._cit_eeprom.bin"

        Logger.start(name=self._testMethodName)

    def tearDown(self):
        run_shell_cmd("rm -f %s" % self.tmp_bios)
        run_shell_cmd("rm -f %s" % self.tmp_eeprom)
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def test_spi_read_bios(self):
        """
        read backup BIOS by spi_util.sh
        """
        cmd = "%s read spi1 BACKUP_BIOS %s" % (self.spi_util, self.tmp_bios)
        run_shell_cmd(cmd)

        self.assertTrue(os.path.exists(self.tmp_bios), "bios file does not exist")

        # Backup BIOS file size should be 16MB
        size = os.stat(self.tmp_bios).st_size
        self.assertTrue(size > 16000000, "bios file size is too small (%d)" % size)

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_spi_read_eeprom(self):
        """
        read oob switch eeprom by spi_util.sh
        """
        cmd = "%s read spi2 OOB_SWITCH_EEPROM %s" % (self.spi_util, self.tmp_eeprom)
        run_shell_cmd(cmd)

        self.assertTrue(os.path.exists(self.tmp_eeprom), "eeprom file does not exist")

        #
        # Sanity check eeprom content and make sure byte order is correct.
        # Check S235340 for more info.
        #
        # Sample "hexdump -C eeprom.bin" output:
        # 00000000  2a a8 01 ff 34 00 01 00  e2 00 01 ff 05 00 02 63
        # 00000010  1f 3e 04 00 01 61 fa 0f  01 60 00 00 01 60 80 00
        # 00000020  ...
        #
        output = run_shell_cmd("hexdump -C %s" % self.tmp_eeprom)
        entries = output.split()
        if entries[1] != "2a" or entries[2] != "a8":
            self.fail("unexepcted eeprom content %s" % output)
