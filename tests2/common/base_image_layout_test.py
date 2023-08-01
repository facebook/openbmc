#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
import json
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseImageLayoutTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.part_infos = {}

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def find_fw_env_mtd(self):
        with open("/etc/fw_env.config") as f:
            data = f.read().splitlines()
            for line in data:
                line = line.strip()
                if line.startswith("#") or line == "":
                    continue
                return line.split()[0].split("/")[-1]
        return ""

    def find_meta_mtd(self, meta_name: str) -> str:
        """
        use CLI cat /proc/mtd to find mtd number
        CLI output will be like:
        mtd0: 00040000 00010000 "romx"
        mtd1: 00010000 00010000 "env"
        mtd2: 00010000 00010000 "meta"
        Locate mtd name (e.g. "mtd2") using meta partition name (e.g. "meta")
        """
        pattern = r'^(?P<mtd_name>mtd[^:]+): .* "{meta_name}"$'.format(
            meta_name=re.escape(meta_name)
        )
        with open("/proc/mtd") as file:
            for _line in file:
                m = re.match(pattern, _line)
                if m:
                    return m.group("mtd_name")
        raise Exception(
            "Could not find partition named {meta_name} in /proc/mtd".format(
                meta_name=repr(meta_name)
            )
        )

    def parsing_dmesg(self, spi_name):
        """
        using dmesg to extract actual image layout
        with CLI: dmesg | grep -E spi0.[01] -A 10
        The output will be like:
        [   12.122532] 8 fixed-partitions partitions found on MTD device spi0.1
        [   12.135329] Creating 8 MTD partitions on "spi0.1":
        [   12.144997] 0x000000000000-0x000000040000 : "romx"
        [   12.163261] random: crng init done
        [   12.173714] 0x0000000e0000-0x0000000f0000 : "env"
        [   12.194252] 0x0000000f0000-0x000000100000 : "meta"
        [   12.216746] 0x000000100000-0x0000001a0000 : "u-boot"
        [   12.239988] 0x0000001a0000-0x000002000000 : "fit"
        [   12.255148] 0x000002000000-0x000002400000 : "data0"
        [   12.270037] 0x000000000000-0x000008000000 : "flash1"
        [   12.284867] 0x000000010000-0x000008000000 : "flash1rw"
        The we parsing the output to extract:
        a. partition name
        b. partition start
        c. partition end
        and store it into self.mtd_part_offsets dict
        """
        cmd = "dmesg | grep -E {} -A 10".format(spi_name)
        _out = run_shell_cmd(cmd).split("\n")
        pattern = (
            r'^\[.*\] 0x(?P<start>[0-9a-f]+)-0x(?P<end>[0-9a-f]+) : "(?P<name>[^"]+)"$'
        )
        self.mtd_part_offsets = {}  # dict: {str: tuple(int, int)}
        for _line in _out:
            m = re.match(pattern, _line)
            if m:
                partition_range = int(m.group("start"), 16), int(m.group("end"), 16)
                partition_name = m.group("name")
                self.mtd_part_offsets[partition_name] = partition_range

    def parsing_meta(self, meta_number):
        """
        using CLI: strings /dev/mtd{x} to read meta info
        the output will be like:
        {
            "FBOBMC_IMAGE_META_VER": 1,
            "meta_update_time": "2020-12-02T17:45:17.100874",
            "meta_update_action": "Signed",
            "part_infos": [
                {
                    "md5": "f37043f6061ecb6420b72bff0eeb04fb",
                    "type": "rom",
                    "name": "spl",
                    "offset": 0,
                    "size": 262144
                },
                {
                    "md5": "d4095443e36d6efb9d0096b9f5707776",
                    "type": "raw",
                    "name": "rec-u-boot",
                    "offset": 262144,
                    "size": 655360
                },
                {
                    "offset": 917504,
                    "type": "data",
                    "name": "u-boot-env",
                    "size": 65536
                },
                {
                    "offset": 983040,
                    "type": "meta",
                    "name": "image-meta",
                    "size": 65536
                },
                {
                    "offset": 1048576,
                    "type": "fit",
                    "num-nodes": 1,
                    "name": "u-boot-fit",
                    "size": 655360
                },
                {
                    "offset": 1703936,
                    "type": "fit",
                    "num-nodes": 3,
                    "name": "os-fit",
                    "size": 31850496
                }
            ],
            "version_infos": {
                "uboot_build_time": "Dec 01 2020 - 19:25:54 +0000",
                "fw_ver": "fby3-v2020.45.2",
                "uboot_ver": "2019.04"
            }
        }
        Then parsing the output to extract:
        a. partition name
        b. partition offset
        c. partition size
        then store it into self.part_infos dict
        """
        with open("/dev/{}".format(meta_number)) as file:
            for line in file:
                j = json.loads(line)
                if "part_infos" in j.keys():
                    self.part_infos = j["part_infos"]
                    break

    def verify_partition(self, dmesg_name, mtd_name):
        """
        use dmesg_name to find partition actual start/end pos
        use mtd_name to find meta's start/end pos (where it supposed to be)
        verify they are matching
        """
        mtd_start_loc = self.mtd_part_offsets[dmesg_name][0]
        mtd_end_loc = self.mtd_part_offsets[dmesg_name][1]
        start_from_meta, end_from_meta = None, None
        for info in self.part_infos:
            if info["name"] == mtd_name:
                start_from_meta = info["offset"]
                end_from_meta = info["offset"] + info["size"]
                break
        self.assertIsNotNone(
            start_from_meta, "Fail to get mtd offset for mtd device {}".format(mtd_name)
        )
        self.assertIsNotNone(
            end_from_meta, "Fail to get mtd size for mtd device {}".format(mtd_name)
        )
        self.assertEqual(start_from_meta, mtd_start_loc, "start loc incosistent")
        self.assertEqual(end_from_meta, mtd_end_loc, "end loc incosistent")

    def do_test(self, spi_name):
        meta_name = self.IMAGE_LAYOUT[spi_name][
            "meta_name"
        ]  # IMAGE_LAYOUT set in the child class
        mtd_num = self.find_meta_mtd(meta_name)
        self.parsing_meta(mtd_num)
        self.parsing_dmesg(spi_name)
        for dname, mtd_name in self.IMAGE_LAYOUT[spi_name]["mapping"].items():
            with self.subTest("checking dmesg:{} meta:{}".format(dname, mtd_name)):
                self.verify_partition(dname, mtd_name)

    def test_spi_0_mtd(self):
        """
        Test actual image layout on spi0.0 matches image meta data
        """
        self.do_test("spi0.0")

    def test_spi_1_mtd(self):
        """
        Test actual image layout on spi0.1 matches image meta data
        """
        self.do_test("spi0.1")

    def test_uboot_tools_part(self):
        env_mtd = self.find_meta_mtd("env")
        fw_mtd = self.find_fw_env_mtd()
        self.assertEqual(env_mtd, fw_mtd, "Ensuring we agree on env partition")
