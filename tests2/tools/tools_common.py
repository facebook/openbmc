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
import sys
import re

class CommonTools(object):
    def dump_py_format(self, info, fp, logger):
        import pprint

        fp.write("info = ")
        fp.write(pprint.pformat(info, indent=4))
        fp.write("\n")

    def dump_json_format(self, info, fp, logger):
        import json

        fp.write(json.dumps(info, sort_keys=True, indent=4))
        fp.write("\n")

    def dump_summary(self, info, logger):
        total = len(info.keys())
        logger.info("Total %d info entries exported" % total)

    def match_single(self, pattern, input):
        """[summary] check if output match regex

        Args:
            pattern ([str]): regex pattern
            input ([str]): string under check

        Returns:
            [bool]: [description]
        """
        p = re.compile(pattern)
        if p.match(input):
            return True
        return False

    def extract_words_within_line(self, start, end, input):
        """[summary] to extract words from line within start/end pattern

        Args:
            start ([str]): start pattern
            end ([str]): end pattern
            input ([str]): [description]

        Returns:
            [str]: [description]
        """
        return re.search(r"\%s(.*?)\%s" % (start, end), input).group(1)


class Log_Simple(object):
    def __init__(self, verbose=False):
        self._verbose = verbose
        self.fp = sys.stdout

    def verbose(self, message):
        if self._verbose:
            print(message, file=self.fp)

    def info(self, message):
        print(message, file=self.fp)
