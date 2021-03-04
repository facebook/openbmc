#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
import subprocess
import sys

from tools_common import CommonTools
from tools_common import Log_Simple


class LogUtilLoader(CommonTools):
    def __init__(self):
        self.info = {}

    def load_log_util_list(self, logger):
        """[summary] to load fw-util FRU/Components info with CLI: fw-util

        Args:
            logger ([type]): [description]

        Returns:
            [dict]: a dict with key: FRU, value: dict of components
        """
        cmd = ["log-util"]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        pattern = "^fru TEXT.*REQUIRED$"
        while True:
            _line = proc.stdout.readline().decode("utf-8").strip()
            if self.match_single(pattern, _line):
                log_types = self.extract_words_within_line("{", "}", _line).split(",")
                logger.verbose("Find log types {}".format(log_types))
                for _log_type in log_types:
                    self.info[_log_type] = {}
                break


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="A tool to generate log-util FRU list")
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="increase output verbosity",
    )
    parser.add_argument(
        "-j",
        "--json-format",
        action="store_true",
        default=False,
        help="generate log-util list info in json format",
    )
    parser.add_argument(
        "-f",
        "--file",
        nargs="?",
        action="store",
        default=None,
        const="info.json",
        help="output list to external file. default: info.json",
    )
    args = parser.parse_args()

    logger = Log_Simple(verbose=args.verbose)
    loader = LogUtilLoader()

    if not args.file:
        logger.fp = sys.stderr
        logger.verbose("writing list info to %s.." % "/dev/stdout")
        outfile = sys.stdout
    else:
        logger.verbose("writing list info to {}".format(args.file))
        outfile = open(args.file, "w")

    loader.load_log_util_list(logger)

    if args.json_format:
        loader.dump_json_format(loader.info, outfile, logger)
    else:
        loader.dump_py_format(loader.info, outfile, logger)

    loader.dump_summary(loader.info, logger)
    logger.info("Commmand completed successfully!")
    sys.exit(0)
