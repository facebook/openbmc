#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

import argparse
import os
import shutil
import subprocess
import tempfile


"""
Steps of this tool
1. scp tests we care about (i.e. platform-specific tests) into the OpenBMC
2. Log into the OpenBMC
3. Run cit_runner
4. Extract valuable information
"""


class OpenBMCTester:
    def __init__(self, hostname, platform, password, port):
        self.tests_base = os.path.dirname(__file__)
        self.platform = platform
        self.hostname = hostname
        self.bmc_start_dir = "/run/tests2"
        self.password = password
        self.port = port

    def scp_to(self, src, dest):
        cmd = [
            "sshpass",
            "-p",
            self.password,
            "scp",
            "-P",
            self.port,
            src,
            f"root@{self.hostname}:{dest}",
        ]
        subprocess.check_call(cmd)

    def remcmd(self, rcmd):
        cmd = [
            "sshpass",
            "-p",
            self.password,
            "ssh",
            "-p",
            self.port,
            f"root@{self.hostname}",
        ] + rcmd
        subprocess.check_call(cmd)

    def upload(self):
        common = [
            "cit_runner.py",
            "common",
            "__init__.py",
            "qemu_denylist.txt",
            "utils",
        ]
        with tempfile.TemporaryDirectory() as base:
            tests2 = os.path.join(base, "tests2")
            os.mkdir(tests2)
            for c in common:
                src = os.path.join(self.tests_base, c)
                dst = os.path.join(tests2, c)
                if os.path.isdir(src):
                    shutil.copytree(src, dst)
                else:
                    shutil.copy(src, dst)
            tests = os.path.join(tests2, "tests")
            os.mkdir(tests)
            plat_test_dst = os.path.join(tests, self.platform)
            plat_test_src = os.path.join(self.tests_base, "tests", self.platform)
            shutil.copytree(plat_test_src, plat_test_dst)

            tests_tar = "tests2.tar.gz"
            subprocess.check_call(["tar", "-zcf", tests_tar, "tests2/"], cwd=base)
            self.scp_to(os.path.join(base, tests_tar), "/run/")
            self.remcmd(["tar", "-xf", "/run/tests2.tar.gz", "-C", "/run/"])
            self.remcmd(["rm", "-f", "/run/tests2.tar.gz"])

    def run(self):
        # Port 2222 is for Qemu.
        if self.port == "2222":
            self.remcmd(["touch", "/run/tests2/dummy_qemu"])
        self.remcmd(
            ["python3", "/run/tests2/cit_runner.py", "--platform", self.platform]
        )


def find_platforms():
    tests_dir = os.path.join(os.path.dirname(__file__), "tests")
    return [
        f for f in os.listdir(tests_dir) if os.path.isdir(os.path.join(tests_dir, f))
    ]


def arg_parse():
    parser = argparse.ArgumentParser()

    parser.add_argument("--hostname", help="Hostname of the BMC to scp")
    parser.add_argument("--port", default="22", help="Port of BMC")
    parser.add_argument("--password", default="0penBmc", help="Password of BMC")

    parser.add_argument(
        "--skip-scp", action="store_true", help="Don't scp tests to OpenBMC"
    )

    parser.add_argument(
        "--platform",
        help="Run all tests in platform by platform name",
        choices=find_platforms(),
    )

    return parser.parse_args()


if __name__ == "__main__":
    args = arg_parse()
    tester = OpenBMCTester(args.hostname, args.platform, args.password, args.port)
    tester.upload()
    tester.run()
