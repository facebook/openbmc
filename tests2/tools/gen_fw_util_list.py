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
from collections import OrderedDict


class Log_Simple:
    def __init__(self, verbose=False):
        self._verbose = verbose
        self.fp = sys.stdout

    def verbose(self, message):
        if self._verbose:
            print(message, file=self.fp)

    def info(self, message):
        print(message, file=self.fp)


class FwUtilLoader(object):
    def __init__(self):
        self.fw_util_info = {}

    def load_fw_util_list(self, logger):
        """[summary] to load fw-util FRU/Components info with CLI: fw-util

        Args:
            logger ([type]): [description]

        Returns:
            [dict]: a dict with key: FRU, value: dict of components
        """
        cmd = ["fw-util"]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        proc_out = OrderedDict()
        while True:
            _line = proc.stdout.readline().decode("utf-8")
            if not _line:
                break
            line = _line.split(":")
            if len(line) == 2:
                proc_out[line[0].strip()] = line[1].strip()
        proc_out = list(proc_out.items())
        cnt = 0
        for item in proc_out:
            k, v = item[0], item[1]
            if k == "FRU" and v == "Components":
                cnt += 2
                break
            cnt += 1
        for idx in range(cnt, len(proc_out)):
            logger.verbose(
                "FRU {} Components {}".format(proc_out[idx][0], proc_out[idx][1])
            )
            # use dict not a list so in future we can use it to store components
            # related test info like regex
            components = {}
            for comp in proc_out[idx][1].split():
                components[comp] = {}
            self.fw_util_info[proc_out[idx][0]] = components

    def find_version_header(self, logger):
        if not self.fw_util_info:
            logger.verbose("fw-util info is empty")
            return
        for fru, comps in self.fw_util_info.items():
            for comp_name in comps.keys():
                cmd = ["/usr/bin/fw-util", fru, "--version", comp_name]
                out = subprocess.check_output(cmd).decode("utf-8")
                version_header = out.split(":")[0].strip()
                logger.verbose(
                    "fru {} components {} version header is {}".format(
                        fru, comp_name, version_header
                    )
                )
                self.fw_util_info[fru][comp_name]["version header"] = version_header

    def dump_fw_util_py_format(self, fw_util_info, fp, logger):
        import pprint

        fp.write("fw_util_info = ")
        fp.write(pprint.pformat(fw_util_info, indent=4))
        fp.write("\n")

    def dump_fw_util_json_format(self, fw_util_info, fp, logger):
        import json

        fp.write(json.dumps(fw_util_info, sort_keys=True, indent=4))
        fp.write("\n")

    def dump_summary(self, fw_util_info, logger):
        total = len(fw_util_info.keys())
        logger.info("Total %d fw_util entries exported" % total)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="A tool to generate fw-util list of mapping between FRU and Components"
    )
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
        help="generate fw-util list info in json format",
    )
    parser.add_argument(
        "-f",
        "--file",
        action="store_true",
        default=False,
        help="output list to fw_util_list.json",
    )
    args = parser.parse_args()

    logger = Log_Simple(verbose=args.verbose)
    fwUtilLoader = FwUtilLoader()

    if not args.file:
        logger.fp = sys.stderr
        logger.verbose("writing fw-util list info to %s.." % "/dev/stdout")
        outfile = sys.stdout
    else:
        logger.verbose("writing fw-util list info to fw_util_list.json")
        outfile = open("fw_util_list.json", "w")

    fwUtilLoader.load_fw_util_list(logger)
    fwUtilLoader.find_version_header(logger)

    if args.json_format:
        fwUtilLoader.dump_fw_util_json_format(
            fwUtilLoader.fw_util_info, outfile, logger
        )
    else:
        fwUtilLoader.dump_fw_util_py_format(fwUtilLoader.fw_util_info, outfile, logger)

    fwUtilLoader.dump_summary(fwUtilLoader.fw_util_info, logger)
    logger.info("Commmand completed successfully!")
    sys.exit(0)
