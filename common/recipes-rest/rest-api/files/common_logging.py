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

import datetime
import logging
import os
import sys
from typing import Any, Dict

import json_log_formatter


class OpenBMCJSONFormatter(json_log_formatter.JSONFormatter):
    def json_record(
        self, message: str, extra: Dict[str, Any], record: logging.LogRecord
    ) -> Dict[str, Any]:
        # for access logs discard message,all information is already included in extra
        if record.name != "aiohttp.access":
            extra["message"] = message
        else:
            # reformat access log request time to isoformat
            extra["request_time"] = datetime.datetime.strptime(
                extra["request_time"][1:-1], "%d/%b/%Y:%H:%M:%S %z"
            ).isoformat()
        # Include loglevel
        extra["level"] = record.levelname

        if "time" not in extra:
            extra["time"] = datetime.datetime.utcnow()

        if record.exc_info:
            extra["exc_info"] = self.formatException(record.exc_info)
        return extra

    def mutate_json_record(self, json_record: Dict[str, Any]):
        for attr_name, attr in json_record.items():
            if isinstance(attr, datetime.datetime):
                json_record[attr_name] = attr.isoformat()
        return json_record


class JsonSyslogFormatter(OpenBMCJSONFormatter):
    def format(self, record):
        return "rest-api: %s" % (super(JsonSyslogFormatter, self).format(record),)


def get_logger_config(config):
    if os.path.exists("/dev/log"):
        rsyslog_config = {
            "level": "INFO",
            "formatter": "syslog_" + config["logformat"],
            "class": "logging.handlers.SysLogHandler",
            "address": "/dev/log",
        }
    else:
        rsyslog_config = {
            "level": "INFO",
            "formatter": config["logformat"],
            "class": "logging.StreamHandler",
            "stream": sys.stdout,
        }
    LOGGER_CONF = {
        "version": 1,
        "disable_existing_loggers": False,
        "formatters": {
            "default": {"format": "%(message)s"},
            "json": {"()": "common_logging.OpenBMCJSONFormatter"},
            "syslog_json": {"()": "common_logging.JsonSyslogFormatter"},
            "syslog_default": {"format": "rest-api: %(message)s"},
        },
        "handlers": {
            "file": {
                "level": "INFO",
                "formatter": config["logformat"],
                "class": "logging.handlers.RotatingFileHandler",
                "filename": config["logfile"],
                "maxBytes": 1048576,
                "backupCount": 3,
                "encoding": "utf8",
            },
            "syslog": rsyslog_config,
            "stdout": {
                "level": "INFO",
                "formatter": config["logformat"],
                "class": "logging.StreamHandler",
                "stream": sys.stdout,
            },
        },
        "loggers": {
            "": {
                "handlers": [config["loghandler"]],
                "level": "DEBUG",
                "propagate": True,
            }
        },
    }
    return LOGGER_CONF
