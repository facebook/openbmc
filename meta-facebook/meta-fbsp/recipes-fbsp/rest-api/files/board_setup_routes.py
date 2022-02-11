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
    app.router.add_get(sled_shim.path, sled_shim.get_handler)
    app.router.add_post(sled_shim.path, sled_shim.post_handler)

    # Add mb /api/sled/mb
    sled_mb_shim = RestShim(get_node_server_2s(1, "mb"), "/api/sled/mb")
    app.router.add_get(sled_mb_shim.path, sled_mb_shim.get_handler)
    app.router.add_post(sled_mb_shim.path, sled_mb_shim.post_handler)

    # Add /api/sled/mb/fruid end point
    fruid_shim = RestShim(get_node_fruid("mb"), "/api/sled/mb/fruid")
    app.router.add_get(fruid_shim.path, fruid_shim.get_handler)
    app.router.add_post(fruid_shim.path, fruid_shim.post_handler)

    # /api/sled/mb/bmc end point
    bmc_shim = RestShim(get_node_bmc(), "/api/sled/mb/bmc")
    app.router.add_get(bmc_shim.path, bmc_shim.get_handler)
    app.router.add_post(bmc_shim.path, bmc_shim.post_handler)

    # /api/sled/mb/sensors end point
    sled_sensor_shim = RestShim(get_node_sensors("mb"), "/api/sled/mb/sensors")
    app.router.add_get(sled_sensor_shim.path, sled_sensor_shim.get_handler)
    app.router.add_post(sled_sensor_shim.path, sled_sensor_shim.post_handler)

    # /api/sled/mb/logs end point
    sled_logs_shim = RestShim(get_node_logs("mb"), "/api/sled/mb/logs")
    app.router.add_get(sled_logs_shim.path, sled_logs_shim.get_handler)
    app.router.add_post(sled_logs_shim.path, sled_logs_shim.post_handler)

    # Add /api/sled/mezz0 to represent Network Mezzaine card
    mezz0_shim = RestShim(get_node_mezz(), "/api/sled/mezz0")
    app.router.add_get(mezz0_shim.path, mezz0_shim.get_handler)

    # Add /api/sled/mezz0/fruid end point
    mezz0_fruid_shim = RestShim(get_node_fruid("nic0"), "/api/sled/mezz0/fruid")
    app.router.add_get(mezz0_fruid_shim.path, mezz0_fruid_shim.get_handler)

    # /api/sled/mezz0/sensors end point
    mezz0_sensors_shim = RestShim(get_node_sensors("nic0"), "/api/sled/mezz0/sensors")
    app.router.add_get(mezz0_sensors_shim.path, mezz0_sensors_shim.get_handler)
    app.router.add_post(mezz0_sensors_shim.path, mezz0_sensors_shim.post_handler)

    # /api/sled/mezz0/logs end point
    mezz0_logs_shim = RestShim(get_node_logs("nic0"), "/api/sled/mezz0/logs")
    app.router.add_post(mezz0_logs_shim.path, mezz0_logs_shim.get_handler)
    app.router.add_post(mezz0_logs_shim.path, mezz0_logs_shim.post_handler)

    # Add /api/sled/mezz1 to represent Network Mezzaine card
    mezz1_shim = RestShim(get_node_mezz(), "/api/sled/mezz1")
    app.router.add_get(mezz1_shim.path, mezz1_shim.get_handler)

    # Add /api/sled/mezz1/fruid end point
    mezz1_fruid_shim = RestShim(get_node_fruid("nic1"), "/api/sled/mezz1/fruid")
    app.router.add_get(mezz1_fruid_shim.path, mezz1_fruid_shim.get_handler)

    # /api/sled/mezz1/sensors end point
    mezz1_sensors_shim = RestShim(get_node_sensors("nic1"), "/api/sled/mezz1/sensors")
    app.router.add_get(mezz1_sensors_shim.path, mezz1_sensors_shim.get_handler)
    app.router.add_post(mezz1_sensors_shim.path, mezz1_sensors_shim.post_handler)

    # /api/sled/mezz1/logs end point
    mezz1_logs_shim = RestShim(get_node_logs("nic1"), "/api/sled/mezz1/logs")
    app.router.add_post(mezz1_logs_shim.path, mezz1_logs_shim.get_handler)
    app.router.add_post(mezz1_logs_shim.path, mezz1_logs_shim.post_handler)
