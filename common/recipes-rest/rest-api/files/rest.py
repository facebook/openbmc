#!/usr/bin/env python
#
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

from ctypes import *
from bottle import route, run, template, request, response, ServerAdapter
from bottle import abort
from wsgiref.simple_server import make_server, WSGIRequestHandler, WSGIServer
import json
import ssl
import socket
import os
from tree import tree
from node import node
from plat_tree import init_plat_tree

CONSTANTS = {
    'certificate': '/usr/lib/ssl/certs/rest_server.pem',
}

root = init_plat_tree()

# Generic router for incoming requests
@route('/<path:path>', method='ANY')
def url_router(path):
    token = path.split('/')
    # Find the Node
    r = root
    for t in token:
        r = r.getChildByName(t)
        if r == None:
            return r
    c = r.data

    # Handle GET request
    if request.method == 'GET':
        # Gather info/actions directly from respective node
        info = c.getInformation()
        actions = c.getActions()

        # Create list of resources from tree structure
        resources = []
        ca = r.getChildren()
        for t in ca:
            resources.append(t.name)
        result = {'Information': info,
                  'Actions': actions,
                  'Resources': resources }

        return result

    # Handle POST request
    if request.method == 'POST':
        lines = request.body.readlines()
        return c.doAction(json.loads(lines[0].decode()))

    return None

run(host = "::", port = 8080)

# TODO: Test the https connection with proper certificates
# SSL Wrapper for Rest API
class SSLWSGIRefServer(ServerAdapter):
    def run(self, handler):
        if self.quiet:
            class QuietHandler(WSGIRequestHandler):
                def log_request(*args, **kw): pass
            self.options['handler_class'] = QuietHandler

        # IPv6 Support
        server_cls = self.options.get('server_class', WSGIServer)

        if ':' in self.host:
            if getattr(server_cls, 'address_family') == socket.AF_INET:
                class server_cls(server_cls):
                    address_family = socket.AF_INET6

        srv = make_server(self.host, self.port, handler,
                server_class=server_cls, **self.options)
        srv.socket = ssl.wrap_socket (
                srv.socket,
                certfile=CONSTANTS['certificate'],
                server_side=True)
        srv.serve_forever()

# Use SSL if the certificate exists. Otherwise, run without SSL.
if os.access(CONSTANTS['certificate'], os.R_OK):
    run(server=SSLWSGIRefServer(host="::", port=8443))
else:
    run(host = "::", port = 8080)
