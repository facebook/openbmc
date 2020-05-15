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

import asyncio
import getopt
import logging
import logging.config
import os.path
import ssl
import sys

from aiohttp import web
from common_logging import get_logger_config
from common_middlewares import auth_enforcer, jsonerrorhandler
from rest_config import load_acl_provider, parse_config
from setup_plat_routes import setup_plat_routes


configpath = "/etc/rest.cfg"

try:
    opts, args = getopt.getopt(sys.argv[1:], "hc:", ["config="])
except getopt.GetoptError:
    print("rest.py -c <config_file>")
    sys.exit(2)

for opt, arg in opts:
    if opt == "-h":
        print("rest.py -c <config_file>")
        sys.exit()
    elif opt in ("-c", "--config"):
        configpath = arg

config = parse_config(configpath)


logging.config.dictConfig(get_logger_config(config))


servers = []


app = web.Application(middlewares=[jsonerrorhandler, auth_enforcer])
setup_plat_routes(app, config)


loop = asyncio.get_event_loop()
handler = app.make_handler()

servers.extend([loop.create_server(handler, "*", port) for port in config["ports"]])

if config["ssl_certificate"] and os.path.isfile(config["ssl_certificate"]):
    ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    if config["ssl_ca_certificate"]:
        # Set up mutual TLS authentication if config has ssl_ca_certificate
        ssl_context.load_verify_locations(config["ssl_ca_certificate"])
        ssl_context.verify_mode = ssl.CERT_OPTIONAL
    ssl_context.load_cert_chain(
        certfile=config["ssl_certificate"], keyfile=config["ssl_key"]  # May be None
    )
    servers.extend(
        [
            loop.create_server(handler, "*", port, ssl=ssl_context)
            for port in config["ssl_ports"]
        ]
    )


app["acl_provider"] = load_acl_provider(config)

srv = loop.run_until_complete(asyncio.gather(*servers, loop=loop))
try:
    loop.run_forever()
except KeyboardInterrupt:
    pass
finally:
    server_closures = []
    for s in srv:
        s.close()
        server_closures.append(s.wait_closed())
    loop.run_until_complete(asyncio.gather(*server_closures, loop=loop))
    loop.run_until_complete(app.shutdown())
    loop.run_until_complete(handler.shutdown(60.0))
    loop.run_until_complete(app.cleanup())
loop.close()
