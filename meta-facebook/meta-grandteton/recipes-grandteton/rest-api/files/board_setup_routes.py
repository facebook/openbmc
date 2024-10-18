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

import typing as t
from dataclasses import dataclass, field
from enum import Enum

import node_gt as gt

from aiohttp.web import Application

from compute_rest_shim import RestShim
from node import node

from node_bmc import get_node_bmc
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_server_2s
from node_sled import get_node_sled
from pal import pal_get_fru_list
from rest_pal_legacy import pal_get_platform_name

# These /api/server routes do not exist on all platforms, and we only set them
# in REST API if the FRUs are visible in libpal
PLATFORM_SPECIFIC_NODES = {
    "scm": "SCM Board",
    "swb": "Switch Board",
    "hgx": "HGX Board",
    "vpdb": "VPDB Board",
    "hpdb": "HPDB Board",
    "fan_bp1": "FAN BP1 Board",
    "fan_bp2": "FAN BP2 Board",
}


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
                        constructor=lambda: get_node_bmc(),
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
                ],
            ),
        ],
    )

    platform = pal_get_platform_name()
    fru_list = pal_get_fru_list()

    for name, description in PLATFORM_SPECIFIC_NODES.items():
        if name in fru_list:
            info = {"Description": platform + " " + description}
            routes.next_routes.append(
                HttpRoute(
                    path="/api/server/{}".format(name),
                    http_methods=[HttpMethod.GET],
                    constructor=lambda info=info: gt.gtNode(info),
                    next_routes=[
                        HttpRoute(
                            path="/api/server/{}/fruid".format(name),
                            http_methods=[HttpMethod.GET],
                            constructor=lambda name=name: get_node_fruid(name),
                        ),
                        HttpRoute(
                            path="/api/server/{}/sensors".format(name),
                            http_methods=[HttpMethod.GET, HttpMethod.POST],
                            constructor=lambda name=name: get_node_sensors(name),
                        ),
                        HttpRoute(
                            path="/api/server/{}/logs".format(name),
                            http_methods=[HttpMethod.GET, HttpMethod.POST],
                            constructor=lambda name=name: get_node_logs(name),
                        ),
                    ],
                ),
            )

    plat_add_route(app, routes)
