#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

import copy
import datetime
import io
import json
import logging
import logging.config
import os
import unittest
from unittest.mock import patch

from common_logging import get_logger_config


stdout_target = {"loghandler": "stdout"}
file_target = {"loghandler": "file"}
syslog_target = {"loghandler": "syslog"}
json_format = {"logformat": "json"}
default_format = {"logformat": "default"}
logfile_location = {"logfile": "/tmp/otherfile.log"}

default_config = {
    "loghandler": "file",
    "logformat": "default",
    "logfile": "/tmp/rest.log",
}


def merge_configs(overrides):
    cfg = copy.deepcopy(default_config)
    for override in overrides:
        cfg.update(override)
    return cfg


class TestLoggerConfigurator(unittest.TestCase):
    def tearDown(self):
        # reset logger
        logging.disable(logging.NOTSET)

    def test_configurator_configures_correct_handler(self):
        logging.config.dictConfig(get_logger_config(merge_configs([stdout_target])))
        self.assertIsInstance(logging.getLogger().handlers[0], logging.StreamHandler)
        logging.config.dictConfig(get_logger_config(merge_configs([file_target])))
        self.assertIsInstance(
            logging.getLogger().handlers[0], logging.handlers.RotatingFileHandler
        )

    def test_configurator_configures_correct_syslog_handler(self):
        logging.config.dictConfig(get_logger_config(merge_configs([syslog_target])))
        expected_parent = (
            logging.handlers.SysLogHandler
            if os.path.exists("/dev/log")
            else logging.StreamHandler
        )

        self.assertIsInstance(logging.getLogger().handlers[0], expected_parent)

    def test_configurator_configures_correct_formatter(self):
        logging_config = get_logger_config(merge_configs([default_format]))
        self.assertEqual(logging_config["handlers"]["file"]["formatter"], "default")
        logging_config = get_logger_config(merge_configs([json_format]))
        self.assertEqual(logging_config["handlers"]["file"]["formatter"], "json")

    @unittest.skipUnless(os.path.exists("/dev/log"), "Syslog not available")
    def test_configurator_configures_correct_formatter_for_syslog(self):
        logging_config = get_logger_config(
            merge_configs([syslog_target, default_format])
        )
        self.assertEqual(
            logging_config["handlers"]["syslog"]["formatter"], "syslog_default"
        )
        logging_config = get_logger_config(merge_configs([syslog_target, json_format]))
        self.assertEqual(
            logging_config["handlers"]["syslog"]["formatter"], "syslog_json"
        )

    def test_configurator_configures_correct_logfile(self):
        logging_config = get_logger_config(merge_configs([logfile_location]))
        self.assertEqual(
            logging_config["handlers"]["file"]["filename"], "/tmp/otherfile.log"
        )

    def test_json_logger_integration(self):
        with patch("sys.stdout", new=io.StringIO()) as fake_out:
            logging.config.dictConfig(
                get_logger_config(merge_configs([stdout_target, json_format]))
            )
            now = datetime.datetime.utcnow()
            logging.info(
                "testmessage", extra={"time": now}
            )  # force in time for the logged entry
            self.assertEqual(
                {"message": "testmessage", "time": now.isoformat(), "level": "INFO"},
                json.loads(fake_out.getvalue()),
            )

    def test_json_logger_extra_args(self):
        with patch("sys.stdout", new=io.StringIO()) as fake_out:
            logging.config.dictConfig(
                get_logger_config(merge_configs([stdout_target, json_format]))
            )
            now = datetime.datetime.utcnow()
            logging.info(
                "testmessage",
                extra={
                    "time": now,
                    "extraarg": "potatoes",
                    "anothertimefield": now,
                    "anumber": 42,
                },
            )
            self.assertEqual(
                {
                    "message": "testmessage",
                    "extraarg": "potatoes",
                    "anothertimefield": now.isoformat(),
                    "anumber": 42,
                    "level": "INFO",
                    "time": now.isoformat(),
                },
                json.loads(fake_out.getvalue()),
            )
