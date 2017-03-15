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

import syslog
import logging


def clamp(v, minv, maxv):
    if v <= minv:
        return minv
    if v >= maxv:
        return maxv
    return v


class Logger(object):

    @staticmethod
    def info(msg):
        print("INFO: " + msg)
        syslog.syslog(syslog.LOG_INFO, msg)
        logging.info(msg)

    @staticmethod
    def debug(msg):
        print("DEBUG: " + msg)

    @staticmethod
    def warn(msg):
        print("WARNING: " + msg)
        syslog.syslog(syslog.LOG_WARNING, msg)

    @staticmethod
    def error(msg):
        print("ERROR: " + msg)
        syslog.syslog(syslog.LOG_ERR, msg)

    @staticmethod
    def crit(msg):
        print("CRITICAL: " + msg)
        syslog.syslog(syslog.LOG_CRIT, msg)

    @staticmethod
    def start(name):
        syslog.openlog(name)
