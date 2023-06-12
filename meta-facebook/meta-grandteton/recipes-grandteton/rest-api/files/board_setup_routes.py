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
import typing as t
from dataclasses import dataclass, field
from enum import Enum

from aiohttp.web import Application

from compute_rest_shim import RestShim
from node import node
from node_sled import get_node_sled
from node_server import get_node_server_2s
from node_mezz import get_node_mezz
import node_gt as gt

from node_bmc import get_node_bmc
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_sensors import get_node_sensors


class HttpMethod(Enum):
    GET = 1
    POST = 2


@dataclass
class HttpRoute:
    path: str
    http_methods: t.List[HttpMethod] = field(default_factory=list)
    constructor: t.Callable[[], node] = field(default=None)
    next_routes: t.List["HttpRoute"] = field(default_factory=list)


def plat_add_route(app, route: HttpRoute):

    shim = RestShim(route.constructor(), route.path)

    if HttpMethod.GET in route.http_methods:
        app.router.add_get(shim.path, shim.get_handler)
    if HttpMethod.POST in route.http_methods:
        app.router.add_post(shim.path, shim.post_handler)

    for next_route in route.next_routes:
        plat_add_route(app, next_route)


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    routes = HttpRoute(
        path="/api/server",
        http_methods=[HttpMethod.GET, HttpMethod.POST],
        constructor=lambda: get_node_sled(),
        next_routes=[
            HttpRoute(
                path="/api/server/mb",
                http_methods=[HttpMethod.GET, HttpMethod.POST],
                constructor=lambda: get_node_server_2s(1, "mb"),
                next_routes=[
                    HttpRoute(
                        path="/api/server/mb/fruid",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_fruid("mb"),
                    ),
                    HttpRoute(
                        path="/api/server/mb/bmc",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_sled(),
                    ),
                    HttpRoute(
                        path="/api/server/mb/sensors",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_sensors("mb"),
                    ),
                    HttpRoute(
                        path="/api/server/mb/logs",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_logs("mb"),
                    ),
                ],
            ),
            HttpRoute(
                path="/api/server/mezz0",
                http_methods=[HttpMethod.GET],
                constructor=lambda: get_node_mezz(),
                next_routes=[
                    HttpRoute(
                        path="/api/server/mezz0/fruid",
                        http_methods=[HttpMethod.GET],
                        constructor=lambda: get_node_fruid("nic0"),
                    ),
                    HttpRoute(
                        path="/api/server/mezz0/sensors",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_sensors("nic0"),
                    ),
                    HttpRoute(
                        path="/api/server/mezz0/logs",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_logs("nic0"),
                    ),
                ],
            ),
            HttpRoute(
                path="/api/server/mezz1",
                http_methods=[HttpMethod.GET],
                constructor=lambda: get_node_mezz(),
                next_routes=[
                    HttpRoute(
                        path="/api/server/mezz1/fruid",
                        http_methods=[HttpMethod.GET],
                        constructor=lambda: get_node_fruid("nic1"),
                    ),
                    HttpRoute(
                        path="/api/server/mezz1/sensors",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_sensors("nic1"),
                    ),
                    HttpRoute(
                        path="/api/server/mezz1/logs",
                        http_methods=[HttpMethod.GET, HttpMethod.POST],
                        constructor=lambda: get_node_logs("nic1"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/scm",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_scm(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/scm/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("scm"),
                    ),
                    HttpRoute (
                        path = "/api/server/scm/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("scm"),
                    ),
                    HttpRoute (
                        path = "/api/server/scm/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("scm"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/swb",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_swb(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/swb/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("swb"),
                    ),
                    HttpRoute (
                        path = "/api/server/swb/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("swb"),
                    ),
                    HttpRoute (
                        path = "/api/server/swb/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("swb"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/vpdb",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_vpdb(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/vpdb/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("vpdb"),
                    ),
                    HttpRoute (
                        path = "/api/server/vpdb/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("vpdb"),
                    ),
                    HttpRoute (
                        path = "/api/server/vpdb/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("vpdb"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/hpdb",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_hpdb(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/hpdb/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("hpdb"),
                    ),
                    HttpRoute (
                        path = "/api/server/hpdb/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("hpdb"),
                    ),
                    HttpRoute (
                        path = "/api/server/hpdb/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("hpdb"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/fan_bp1",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_fan_bp1(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/fan_bp1/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("fan_bp1"),
                    ),
                    HttpRoute (
                        path = "/api/server/fan_bp1/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("fan_bp1"),
                    ),
                    HttpRoute (
                        path = "/api/server/fan_bp1/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("fan_bp1"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/fan_bp2",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_fan_bp2(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/fan_bp2/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("fan_bp2"),
                    ),
                    HttpRoute (
                        path = "/api/server/fan_bp2/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("fan_bp2"),
                    ),
                    HttpRoute (
                        path = "/api/server/fan_bp2/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("fan_bp2"),
                    ),
                ]
            ),
            HttpRoute (
                path = "/api/server/hgx",
                http_methods = [HttpMethod.GET],
                constructor = lambda: gt.get_node_hgx(),
                next_routes = [
                    HttpRoute (
                        path = "/api/server/hgx/fruid",
                        http_methods = [HttpMethod.GET],
                        constructor = lambda: get_node_fruid("hgx"),
                    ),
                    HttpRoute (
                        path = "/api/server/hgx/sensors",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_sensors("hgx"),
                    ),
                    HttpRoute (
                        path = "/api/server/hgx/logs",
                        http_methods = [HttpMethod.GET, HttpMethod.POST],
                        constructor = lambda: get_node_logs("hgx"),
                    ),
                ]
            ),
        ]
    )

    plat_add_route(app, routes)
