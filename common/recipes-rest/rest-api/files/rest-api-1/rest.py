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

import bottle
from cherrypy.wsgiserver import CherryPyWSGIServer
from cherrypy.wsgiserver.ssl_pyopenssl import pyOpenSSLAdapter
import datetime
import logging
import logging.config
from rest_config import RestConfig
from common_endpoint import commonApp
from board_endpoint import boardApp


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


# SSL Wrapper for Rest API
class SSLCherryPyServer(bottle.ServerAdapter):
    def run(self, handler):
        server = CherryPyWSGIServer((self.host, self.port), handler)
        server.ssl_adapter = \
                pyOpenSSLAdapter(RestConfig.get('ssl', 'certificate'),
                                 RestConfig.get('ssl', 'key'))
        try:
            server.start()
        finally:
            server.stop()


def log_after_request():
    try:
        length = bottle.response.content_length
    except Exception:
        try:
            length = len(bottle.response.body)
        except Exception:
            length = 0

    logging.info('{} - - [{}] "{} {} {}" {} {}'.format(
        bottle.request.environ.get('REMOTE_ADDR'),
        datetime.datetime.now().strftime('%d/%b/%Y %H:%M:%S'),
        bottle.request.environ.get('REQUEST_METHOD'),
        bottle.request.environ.get('REQUEST_URI'),
        bottle.request.environ.get('SERVER_PROTOCOL'),
        bottle.response.status_code,
        length))


# Error logging to log file
class ErrorLogging(object):
    def write(self, err):
        logging.error(err)


# Middleware to log the requests
class LogMiddleware(object):
    def __init__(self, app):
        self.app = app

    def __call__(self, e, h):
        e['wsgi.errors'] = ErrorLogging()
        ret_val = self.app(e, h)
        log_after_request()
        return ret_val

# overwrite the stderr and stdout to log to the file
bottle._stderr = logging.error
bottle._stdout = logging.info
logging.config.dictConfig(LOGGER_CONF)

serverApp = bottle.app()
serverApp.merge(commonApp)
serverApp.merge(boardApp)
bottle_app = LogMiddleware(serverApp)
# Use SSL if the certificate and key exists. Otherwise, run without SSL.
if (RestConfig.getboolean('listen', 'ssl')):
    bottle.run(host="::", port=RestConfig.getint('listen', 'port'),
               server=SSLCherryPyServer, app=bottle_app)
else:
    bottle.run(host="::", port=RestConfig.getint('listen', 'port'),
               server='cherrypy', app=bottle_app)
