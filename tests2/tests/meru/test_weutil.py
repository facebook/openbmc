#!/usr/bin/env python3
#
# Copyright 2023-present Facebook. All Rights Reserved.
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
import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Weutil(unittest.TestCase):
    def test_weutil(self):

        for target in ["bmc", "scm", "smb"]:
            output = run_shell_cmd("weutil -e " + target)
            line = output.split("\n")
            result = True

            for i in range(len(line)):
                if "Wedge EEPROM " in line[i]:
                    field = line[i].split("Wedge EEPROM ")[1]
                    if field != target:
                        result = False
                        break
                if "Version:" in line[i]:
                    field = line[i].split("Version:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Name:" in line[i]:
                    field = line[i].split("Product Name:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Part Number:" in line[i]:
                    field = line[i].split("Product Part Number:")[1]
                    if field is None:
                        result = False
                        break
                if "System Assembly Part Number:" in line[i]:
                    field = line[i].split("System Assembly Part Number:")[1]
                    if field is None:
                        result = False
                        break
                if "Facebook PCBA Part Number:" in line[i]:
                    field = line[i].split("Facebook PCBA Part Number:")[1]
                    if field is None:
                        result = False
                        break
                if "ODM/JDM PCBA Part Number:" in line[i]:
                    field = line[i].split("ODM/JDM PCBA Part Number:")[1]
                    if field is None:
                        result = False
                        break
                if "ODM/JDM PCBA Serial Number:" in line[i]:
                    field = line[i].split("ODM/JDM PCBA Serial Number:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Production State:" in line[i]:
                    field = line[i].split("Product Production State:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Version:" in line[i]:
                    field = line[i].split("Product Version:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Sub-Version:" in line[i]:
                    field = line[i].split("Product Sub-Version:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Serial Number:" in line[i]:
                    field = line[i].split("Product Serial Number:")[1]
                    if field is None:
                        result = False
                        break
                if "Product Asset Tag:" in line[i]:
                    field = line[i].split("Product Asset Tag:")[1]
                    if field is None:
                        result = False
                        break
                if "System Manufacturer:" in line[i]:
                    field = line[i].split("System Manufacturer:")[1]
                    if field is None:
                        result = False
                        break
                if "System Manufacturing Date:" in line[i]:
                    field = line[i].split("System Manufacturing Date:")[1]
                    if field is None:
                        result = False
                        break
                if "PCB Manufacturer:" in line[i]:
                    field = line[i].split("PCB Manufacturer:")[1]
                    if field is None:
                        result = False
                        break
                if "Assembled At:" in line[i]:
                    field = line[i].split("Assembled At:")[1]
                    if field is None:
                        result = False
                        break
                if "Local MAC:" in line[i]:
                    field = line[i].split("Local MAC:")[1]
                    if field is None:
                        result = False
                        break
                if "Extended MAC Base:" in line[i]:
                    field = line[i].split("Extended MAC Base:")[1]
                    if field is None:
                        result = False
                        break
                if "Extended MAC Address Size:" in line[i]:
                    field = line[i].split("Extended MAC Address Size:")[1]
                    if field is None:
                        result = False
                        break
                if "Location on Fabric:" in line[i]:
                    field = line[i].split("Location on Fabric:")[1]
                    if field is None:
                        result = False
                        break
                if "CRC16:" in line[i]:
                    field = line[i].split("CRC16:")[1]
                    if field is None:
                        result = False
                        break

            if result is False:
                print("The following field is empty")
                print(line[i])

            self.assertEqual(
                result,
                True,
                "weutil -e " + target + " check is failed ",
        )
