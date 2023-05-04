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


import json
import re

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BaseLogUtilTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.set_fru_id()

    def validate_fru_id(self, fru_id, regex=None):
        """
        validate fru_id field, it should contain digit only
        ex. 0,9
        """
        if not regex:
            regex = r"\d+"
        self.assertRegex(fru_id, regex, "incorrect fru id found: {}".format(fru_id))

    def validate_timestamp(self, ts, regex=None):
        """
        validate timestamp format
        ex. 2021-06-04 12:11:12
        """
        if not regex:
            regex = r"\d{1,4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}"
        self.assertRegex(ts, regex, "incorrect timstamp found: {}".format(ts))

    def validate_app_name(self, app_name, regex=None):
        """
        validate app name, it should not contain any special chracter
        """
        if not regex:
            regex = r"[a-zA-Z0-9]"
        self.assertRegex(
            app_name, regex, "incorrect app name found: {}".format(app_name)
        )

    def set_fru_id(self):
        """
        for spacific FRU need to override this function
        to define FRU_ID with fru id number by integer type
        """
        self.FRU_ID = None

    def test_log_format(self):
        """
        test log-util [FRU] --print and verify output
        """
        cmd = ["log-util", self.FRU, "--print"]
        Logger.info("running cmd: {}".format(" ".join(cmd)))
        out = run_cmd(cmd)
        if not out:
            Logger.info("empty log, skip the test")
            self.assertTrue(True)
            return
        pattern = r"(?P<fru_id>\S+)\s{2,}(?P<fru_name>\S+)\s{2,}(?P<ts>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\s{4,}(?P<app_name>\S+)\s{2,}(?P<msg>.+)[\n]+"
        m = re.search(pattern, out)
        if m:
            self.validate_fru_id(m.group("fru_id"))
            self.validate_timestamp(m.group("ts"))
            self.validate_app_name(m.group("app_name"))
        else:
            if re.search(r"User cleared", out):
                Logger.info("Log being cleared by user, skip the test")
                self.assertTrue(True)
                return
            else:
                self.assertIsNotNone(
                    m, "no matching log found. output is {}".format(out)
                )

    def test_log_format_json(self):
        """
        test log-util [FRU] --print --json and verify output
        """
        cmd = ["log-util", self.FRU, "--print", "--json"]
        Logger.info("running cmd: {}".format(" ".join(cmd)))
        out = run_cmd(cmd)
        if not out:
            Logger.info("empty output, skip the test")
            self.assertTrue(True)
            return
        # convert json out to dict
        out = json.loads(out)
        # if log is empty, skip the test
        if len(out["Logs"]) == 0:
            Logger.info("empty log, skip test")
            self.assertTrue(True)
            return
        # extract the first logs from the output
        log = out["Logs"][0]
        # validate FRU_ID
        self.validate_fru_id(log["FRU#"])
        # validate FRU_NAME
        # if FRU_NAME is "all", then skip FRU check since it could be anything
        if self.FRU != "all":
            self.assertEqual(
                log["FRU_NAME"],
                self.FRU,
                "unmatch fru, expect {} got {}".format(self.FRU, log["FRU_NAME"]),
            )
        # validate timestamp
        self.validate_timestamp(log["TIME_STAMP"])
        # validate app name
        self.validate_app_name(log["APP_NAME"])

    def test_log_clear(self):
        """
        test to clear SEL
        we only test to clear sys log so:
        1. speed up the test
        2. won't clear whole BMC logs
        """
        cmd = ["log-util", self.FRU, "--clear"]
        Logger.info("running cmd: {}".format(" ".join(cmd)))
        out = run_cmd(cmd)
        cmd = ["log-util", self.FRU, "--print"]

        """
        the output log-util will show with 'FRU: {FRU_ID}'
        for spacific testing need to define FRU_ID for each fru
        except 'all' and 'sys'
        """
        if self.FRU_ID is not None:
            pattern = r"log-util: User cleared FRU: " + str(self.FRU_ID) + r" logs"
        else:
            pattern = r"log-util: User cleared " + self.FRU + r" logs"

        out = run_cmd(cmd)
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))
