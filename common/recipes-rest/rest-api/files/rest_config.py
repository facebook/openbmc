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

import configparser
import syslog


def parse_config(configpath):
    RestConfig = configparser.ConfigParser()
    RestConfig.read(configpath)

    writable = RestConfig.getboolean("access", "write", fallback=False)
    if writable:
        syslog.syslog(syslog.LOG_INFO, "REST: Launched with Read/Write Mode")
    else:
        syslog.syslog(syslog.LOG_INFO, "REST: Launched with Read Only Mode")

    return {
        "ports": RestConfig.get("listen", "port", fallback="8080").split(","),
        "ssl_ports": list(
            filter(None, RestConfig.get("listen", "ssl_port", fallback="").split(","))
        ),
        "logfile": RestConfig.get("logging", "filename", fallback="/tmp/rest.log"),
        "writable": writable,
        "ssl_certificate": RestConfig.get("ssl", "certificate", fallback=None),
        "ssl_key": RestConfig.get("ssl", "key", fallback=None),
    }
