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

from aiohttp.web import Application
from compute_rest_shim import RestShim
from node_bmc import get_node_bmc
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_server_2s
from node_sled import get_node_sled


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Add /api/sled to represent entire SLED
    sled_shim = RestShim(get_node_sled(), "/api/sled")
    app.router.add_get("/api/sled", sled_shim.get_handler)
    app.router.add_post("/api/sled", sled_shim.post_handler)

    # Add mb /api/sled/mb
    sled_shim = RestShim(get_node_server_2s(1, "mb"), "/api/sled/mb")
    app.router.add_get("/api/sled/mb", sled_shim.get_handler)
    app.router.add_post("/api/sled/mb", sled_shim.post_handler)

    # Add /api/sled/mb/fruid end point
    fruid_shim = RestShim(get_node_fruid("mb"), "/api/sled/mb/fruid")
    app.router.add_get("/api/sled/mb/fruid", fruid_shim.get_handler)
    app.router.add_post("/api/sled/mb/fruid", fruid_shim.post_handler)

    # /api/sled/mb/bmc end point
    bmc_shim = RestShim(get_node_bmc(), "/api/sled/mb/bmc")
    app.router.add_get("/api/sled/mb/bmc", bmc_shim.get_handler)
    app.router.add_post("/api/sled/mb/bmc", bmc_shim.post_handler)

    # /api/sled/mb/sensors end point
    sled_sensor_shim = RestShim(get_node_sensors("mb"), "/api/sled/mb/sensors")
    app.router.add_get("/api/sled/mb/sensors", sled_sensor_shim.get_handler)
    app.router.add_post("/api/sled/mb/sensors", sled_sensor_shim.post_handler)

    # /api/sled/mb/logs end point
    sled_logs_shim = RestShim(get_node_logs("mb"), "/api/sled/mb/logs")
    app.router.add_get("/api/sled/mb/logs", sled_logs_shim.get_handler)
    app.router.add_post("/api/sled/mb/logs", sled_logs_shim.post_handler)

    # Add /api/sled/mezz to represent Network Mezzaine card
    mezz_shim = RestShim(get_node_mezz(), "/api/sled/mezz")
    app.router.add_get("/api/sled/mezz", mezz_shim.get_handler)

    # Add /api/sled/mezz/fruid end point
    mezz_fruid_shim = RestShim(get_node_fruid("nic"), "/api/sled/mezz/fruid")
    app.router.add_get("/api/sled/mezz/fruid", mezz_fruid_shim.get_handler)

    # /api/sled/mezz/sensors end point
    mezz_fruid_shim = RestShim(get_node_sensors("nic"), "/api/sled/mezz/sensors")
    app.router.add_get("/api/sled/mezz/sensors", mezz_fruid_shim.get_handler)
    app.router.add_post("/api/sled/mezz/sensors", mezz_fruid_shim.post_handler)

    # /api/sled/mezz/logs end point
    mezz_logs_shim = RestShim(get_node_logs("nic"), "/api/sled/mezz/logs")
    app.router.add_post("/api/sled/mezz/logs", mezz_logs_shim.post_handler)
    app.router.add_post("/api/sled/mezz/logs", mezz_logs_shim.post_handler)
