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

import json
from aiohttp.web import Application
from compute_rest_shim import RestShim
from node_bmc import get_node_bmc
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_server_2s
from node_sled import get_node_sled

HTTP_GET  = 1
HTTP_POST = 2
HTTP_BOTH = HTTP_GET + HTTP_POST

def plat_add_route(app, node):

    shim = RestShim(node["construct_func"](*node["construct_func_args"]), node["path"])

    if node["http_func"] == HTTP_GET:
        app.router.add_get(shim.path, shim.get_handler)
    elif node["http_func"] == HTTP_POST:
        app.router.add_post(shim.path, shim.post_handler)
    elif node["http_func"] == HTTP_BOTH:
        app.router.add_get(shim.path, shim.get_handler)
        app.router.add_post(shim.path, shim.post_handler)

    for nextnode in node["next"]:
        plat_add_route(app, node["next"][nextnode])

# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    routes = {
        "path": "/api/server",
        "http_func": HTTP_BOTH,
        "construct_func": get_node_sled,
        "construct_func_args": [],
        "next": {
            "mb": {
                "path": "/api/server/mb",
                "http_func": HTTP_BOTH,
                "construct_func": get_node_server_2s,
                "construct_func_args": [1, "mb"],
                "next": {
                    "fruid": {
                        "path": "/api/server/mb/fruid",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_fruid,
                        "construct_func_args": ["mb"],
                        "next": {},
                    },
                    "bmc": {
                        "path": "/api/server/mb/bmc",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_sled,
                        "construct_func_args": [],
                        "next": {},
                    },
                    "sensors":{
                        "path": "/api/server/mb/sensors",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_sensors,
                        "construct_func_args": ["mb"],
                        "next": {},
                    },
                    "logs":{
                        "path": "/api/server/mb/logs",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_logs,
                        "construct_func_args": ["mb"],
                        "next": {},
                    },
                },
            },
            "mezz0":{
                "path": "/api/server/mezz0",
                "http_func": HTTP_GET,
                "construct_func": get_node_mezz,
                "construct_func_args": [],
                "next": {
                    "fruid": {
                        "path": "/api/server/mezz0/fruid",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_fruid,
                        "construct_func_args": ["nic0"],
                        "next": {},
                    },
                    "sensors":{
                        "path": "/api/server/mezz0/sensors",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_sensors,
                        "construct_func_args": ["nic0"],
                        "next": {},
                    },
                    "logs":{
                        "path": "/api/server/mezz0/logs",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_logs,
                        "construct_func_args": ["nic0"],
                        "next": {},
                    },
                },
            },
            "mezz1":{
                "path": "/api/server",
                "http_func": HTTP_GET,
                "construct_func": get_node_mezz,
                "construct_func_args": [],
                "next": {
                    "fruid": {
                        "path": "/api/server/mezz1/fruid",
                        "http_func": HTTP_GET,
                        "construct_func": get_node_fruid,
                        "construct_func_args": ["nic1"],
                        "next": {}
                    },
                    "sensors":{
                        "path": "/api/server/mezz1/sensors",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_sensors,
                        "construct_func_args": ["nic1"],
                        "next": {}
                    },
                    "logs":{
                        "path": "/api/server/mezz1/logs",
                        "http_func": HTTP_BOTH,
                        "construct_func": get_node_logs,
                        "construct_func_args": ["nic1"],
                        "next": {}
                    },
                }
            },
        }
    }

    plat_add_route(app, routes)

