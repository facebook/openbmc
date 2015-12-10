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
import json
import ssl
import socket
import os
import rest_fruid
import rest_server
import rest_sensors
import rest_bmc
import rest_gpios
import rest_modbus
import rest_slotid
import rest_psu_update

CONSTANTS = {
    'certificate': '/usr/lib/ssl/certs/rest_server.pem',
    'key': '/usr/lib/ssl/private/rest_server_key.pem',
}

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
            'formatter':'default',
            'class': 'logging.handlers.RotatingFileHandler',
            'filename':'/tmp/rest.log',
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

# Handler for root resource endpoint
@bottle.route('/api')
def rest_api():
   result = {
                "Information": {
                    "Description": "Wedge RESTful API Entry",
                    },
                "Actions": [],
                "Resources": [ "sys"],
             }

   return result

# Handler for sys resource endpoint
@bottle.route('/api/sys')
def rest_sys():
    result = {
                "Information": {
                    "Description": "Wedge System",
                    },
                "Actions": [],
                "Resources": [ "mb", "bmc", "server", "sensors", "gpios",
                               "modbus_registers", "slotid"],
             }

    return result

# Handler for sys/mb resource endpoint
@bottle.route('/api/sys/mb')
def rest_sys():
    result = {
                "Information": {
                    "Description": "System Motherboard",
                    },
                "Actions": [],
                "Resources": [ "fruid"],
             }

    return result

# Handler for sys/mb/fruid resource endpoint
@bottle.route('/api/sys/mb/fruid')
def rest_fruid_hdl():
  return rest_fruid.get_fruid()

# Handler for sys/bmc resource endpoint
@bottle.route('/api/sys/bmc')
def rest_bmc_hdl():
    return rest_bmc.get_bmc()

# Handler for sys/server resource endpoint
@bottle.route('/api/sys/server')
def rest_server_hdl():
    return rest_server.get_server()

# Handler for uServer resource endpoint
@bottle.route('/api/sys/server', method='POST')
def rest_server_act_hdl():
    data = json.load(request.body)
    return rest_server.server_action(data)

# Handler for sensors resource endpoint
@bottle.route('/api/sys/sensors')
def rest_sensors_hdl():
  return rest_sensors.get_sensors()

# Handler for sensors resource endpoint
@bottle.route('/api/sys/gpios')
def rest_gpios_hdl():
  return rest_gpios.get_gpios()

@bottle.route('/api/sys/modbus_registers')
def modbus_registers_hdl():
    return rest_modbus.get_modbus_registers()

@bottle.route('/api/sys/psu_update')
def psu_update_hdl():
    return rest_psu_update.get_jobs()

@bottle.route('/api/sys/psu_update', method='POST')
def psu_update_hdl():
    data = json.load(request.body)
    return rest_psu_update.begin_job(data)

# Handler for sensors resource endpoint
@bottle.route('/api/sys/slotid')
def rest_slotid_hdl():
  return rest_slotid.get_slotid()

# SSL Wrapper for Rest API
class SSLCherryPyServer(bottle.ServerAdapter):
    def run(self, handler):
        server = CherryPyWSGIServer((self.host, self.port), handler)
        server.ssl_adapter = pyOpenSSLAdapter(CONSTANTS['certificate'], CONSTANTS['key'])
        try:
            server.start()
        finally:
            server.stop()

def log_after_request():
    try:
        length = bottle.response.content_length
    except:
        try:
            length = len(bottle.response.body)
        except:
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

bottle_app = LogMiddleware(bottle.app())
# Use SSL if the certificate and key exists. Otherwise, run without SSL.
if (os.access(CONSTANTS['key'], os.R_OK) and
    os.access(CONSTANTS['certificate'], os.R_OK)):
    bottle.run(host = "::", port= 8443, server=SSLCherryPyServer, app=bottle_app)
else:
    bottle.run(host = "::", port = 8080, server='cherrypy', app=bottle_app)
