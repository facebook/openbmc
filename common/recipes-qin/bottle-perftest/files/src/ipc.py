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

import logging
import logging.config
import urllib2

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
            "filename": "/tmp/rest_ipc.log",
            "maxBytes": 1048576,
            "backupCount": 3,
            "encoding": "utf8",
        }
    },
    "loggers": {
        "": {"handlers": ["file_handler"], "level": "DEBUG", "propagate": True}
    },
}

# Handler for ipc resource endpoint
@bottle.route("/ipc")
def rest_ipc():
    result = {
        "Information": {"Description": "RESTful API Entry for IPC"},
        "Actions": [],
        "Resources": ["service0"],
    }

    return result


# Handler for service0 resource endpoint
# Here it http gets the url from the service at http://localhost:8047/ping
@bottle.route("/ipc/service0")
def rest_service():
    return urllib2.urlopen("http://localhost:8047/ping").read()


bottle._stderr = logging.error
bottle._stdout = logging.info
logging.config.dictConfig(LOGGER_CONF)

# create server at localhost
bottle.run(port=8043, server="cherrypy")
