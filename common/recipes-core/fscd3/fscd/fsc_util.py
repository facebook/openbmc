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
import syslog


LOGGER_CONF = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {
        "default": {"format": "%(asctime)s %(message)s", "datefmt": "%Y-%m-%d %H:%M:%S"}
    },
    "handlers": {
        "file_handler": {
            "level": "INFO",
            "formatter": "default",
            "class": "logging.handlers.RotatingFileHandler",
            "filename": "/var/log/fscd.log",
            "maxBytes": 100000,
            "backupCount": 1,
            "encoding": "utf8",
        }
    },
    "loggers": {
        "": {"handlers": ["file_handler"], "level": "DEBUG", "propagate": True}
    },
}

LOG_MAP = {
    "debug": syslog.LOG_DEBUG,
    "info": syslog.LOG_INFO,
    "warning": syslog.LOG_WARNING,
    "error": syslog.LOG_ERR,
    "critical": syslog.LOG_CRIT,
}


def clamp(v, minv, maxv):
    if v <= minv:
        return minv
    if v >= maxv:
        return maxv
    return v


class Logger(object):
    @staticmethod
    def info(msg):
        # print("INFO: " + msg)
        syslog.syslog(syslog.LOG_INFO, msg)
        logging.info(msg)

    @staticmethod
    def debug(msg):
        # print("DEBUG: " + msg)
        syslog.syslog(syslog.LOG_DEBUG, msg)

    @staticmethod
    def warn(msg):
        # print("WARNING: " + msg)
        syslog.syslog(syslog.LOG_WARNING, msg)

    @staticmethod
    def error(msg):
        # print("ERROR: " + msg)
        syslog.syslog(syslog.LOG_ERR, msg)

    @staticmethod
    def crit(msg):
        # print("CRITICAL: " + msg)
        syslog.syslog(syslog.LOG_CRIT, msg)

    @staticmethod
    def usbdbg(msg):
        # print("USBDBG: " + msg)
        syslog.syslog((syslog.LOG_LOCAL0 | syslog.LOG_ERR), msg)

    @staticmethod
    def start(name, log_level):
        syslog.openlog(name)
        syslog.setlogmask(syslog.LOG_UPTO(LOG_MAP[log_level]))
        logging.config.dictConfig(LOGGER_CONF)
