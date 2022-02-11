# Copyright 2015-present Facebook. All Rights Reserved.
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

import logging
import logging.config


LOGGER_CONF = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {"default": {"format": "%(message)s"}},
    "handlers": {
        "file_handler": {
            "level": "INFO",
            "formatter": "default",
            "class": "logging.handlers.RotatingFileHandler",
            "filename": "/tmp/cit.log",
            "maxBytes": 100000,
            "backupCount": 1,
            "encoding": "utf8",
        }
    },
    "loggers": {
        "": {"handlers": ["file_handler"], "level": "DEBUG", "propagate": True}
    },
}


class Logger(object):
    @staticmethod
    def info(msg):
        logging.info(msg)

    @staticmethod
    def debug(msg):
        logging.debug(msg)

    @staticmethod
    def warn(msg):
        logging.warn(msg)

    @staticmethod
    def error(msg):
        logging.error(msg)

    @staticmethod
    def crit(msg):
        syslog.syslog(syslog.LOG_CRIT, msg)

    @staticmethod
    def start(name):
        logging.config.dictConfig(LOGGER_CONF)
        logging.info("Starting cit logging for " + name)

    def log_testname(name):
        logging.info("Running test " + name)
