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
import syslog

from aiohttp import web
from common_tree import init_common_tree
from common_middlewares import jsonerrorhandler
from node import node
from plat_tree import init_plat_tree
from rest_config import parse_config
from tree import tree


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

LOGGER_CONF = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {"default": {"format": "%(message)s"}},
    "handlers": {
        "file_handler": {
            "level": "INFO",
            "formatter": "default",
            "class": "logging.handlers.RotatingFileHandler",
            "filename": config["logfile"],
            "maxBytes": 1048576,
            "backupCount": 3,
            "encoding": "utf8",
        }
    },
    "loggers": {
        "": {"handlers": ["file_handler"], "level": "DEBUG", "propagate": True}
    },
}
logging.config.dictConfig(LOGGER_CONF)


servers = []


app = web.Application(middlewares=[jsonerrorhandler])
if config["writable"]:
    syslog.syslog(syslog.LOG_INFO, "REST: Launched with Read/Write Mode")
else:
    syslog.syslog(syslog.LOG_INFO, "REST: Launched with Read Only Mode")

root = init_common_tree()
root.merge(init_plat_tree())
root.setup(app, config["writable"])


loop = asyncio.get_event_loop()
handler = app.make_handler()

servers.extend([loop.create_server(handler, "*", port) for port in config["ports"]])

if config["ssl_certificate"] and os.path.isfile(config["ssl_certificate"]):
    ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    ssl_context.load_cert_chain(
        certfile=config["ssl_certificate"], keyfile=config["ssl_key"]  # May be None
    )
    servers.extend(
        [
            loop.create_server(handler, "*", port, ssl=ssl_context)
            for port in config["ssl_ports"]
        ]
    )

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
