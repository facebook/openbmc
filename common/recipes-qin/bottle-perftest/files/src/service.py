#!/usr/bin/env python
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
import logging.config

import bottle
from cherrypy.wsgiserver import CherryPyWSGIServer


# This setting is to direct stdout and stderr of the bottle
# to the log file.
LOGGER_CONF = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {"default": {"format": "%(message)s"}},
    "handlers": {
        "file_handler": {
            "level": "INFO",
            "formatter": "default",
            "class": "logging.handlers.RotatingFileHandler",
            "filename": "/tmp/rest_service.log",
            "maxBytes": 1048576,
            "backupCount": 3,
            "encoding": "utf8",
        }
    },
    "loggers": {
        "": {"handlers": ["file_handler"], "level": "DEBUG", "propagate": True}
    },
}

# Handler for root resource endpoint
# Here it responds the http get with a timestamp
@bottle.route("/ping")
def rest_ping():
    result = "Ping received at " + datetime.datetime.now().strftime("%d/%b/%Y %H:%M:%S")
    return result


bottle._stderr = logging.error
bottle._stdout = logging.info
logging.config.dictConfig(LOGGER_CONF)

# create server at localhost
bottle.run(host="localhost", port=8047, server="cherrypy")
