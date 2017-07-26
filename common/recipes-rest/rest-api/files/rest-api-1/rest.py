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
from common_setup_routes import setup_common_routes
from board_setup_routes import setup_board_routes
import logging
import logging.config
import ssl

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


app = web.Application()
setup_common_routes(app)
setup_board_routes(app)
logging.config.dictConfig(LOGGER_CONF)

web.run_app(app, host="::")

