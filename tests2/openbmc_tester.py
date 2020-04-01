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


"""
Steps of this tool
1. scp tests we care about (i.e. platform-specific tests) into the OpenBMC
2. Log into the OpenBMC
3. Run cit_runner
4. Extract valuable information
"""


class OpenBMCTester:
    def __init__(self, hostname, platform):
        self.platform = platform
        self.hostname = hostname
        self.bmc_start_dir = "/usr/local/bin/tests2"


def arg_parse():
    parser = argparse.ArgumentParser()

    parser.add_argument("--hostname", help="Hostname of the BMC to scp")

    parser.add_argument(
        "--skip-scp", action="store_true", help="Don't scp tests to OpenBMC"
    )

    parser.add_argument(
        "--platform",
        help="Run all tests in platform by platform name",
        choices=[
            "wedge",
            "wedge100",
            "galaxy100",
            "cmm",
            "minipack",
            "fbtp",
            "fby2",
            "fbttn",
            "fuji",
            "yamp",
        ],
    )

    return parser.parse_args()


if __name__ == "__main__":
    args = arg_parse()
    tester = OpenBMCTester(args.hostname, args.platform)
