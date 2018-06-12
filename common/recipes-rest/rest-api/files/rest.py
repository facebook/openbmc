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

# aiohttp has access logs and error logs enabled by default. So, only the configuration file is needed to get the desired log output.

from aiohttp import web
import logging
import logging.config
import syslog
import ssl
from tree import tree
from node import node
from plat_tree import init_plat_tree
from rest_config import RestConfig

LOGGER_CONF = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters': {
        'default': {
            'format': '%(message)s'
        },
    },
    'handlers': {
        'file_handler': {
            'level': 'INFO',
            'formatter': 'default',
            'class': 'logging.handlers.RotatingFileHandler',
            'filename': '/tmp/rest.log',
            'maxBytes': 1048576,
            'backupCount': 3,
            'encoding': 'utf8'
        },
    },
    'loggers': {
        '': {
            'handlers': ['file_handler'],
            'level': 'DEBUG',
            'propagate': True,
        },
    }
}

logging.config.dictConfig(LOGGER_CONF)
app = web.Application()
is_read_only = not RestConfig.getboolean('access', 'write', fallback=False)
if is_read_only:
    syslog.syslog(syslog.LOG_INFO, 'REST: Launched with Read Only Mode')
else:
    syslog.syslog(syslog.LOG_INFO, 'REST: Launched with Read/Write Mode')
root = init_plat_tree(is_read_only)
root.setup(app, not is_read_only)
web.run_app(app)

