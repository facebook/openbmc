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

import os

from aiohttp.web import Application
from compute_rest_shim import RestShim
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_server
from node_spb import get_node_spb
from rest_pal_legacy import pal_is_fru_prsnt, pal_get_num_slots


def populate_server_node(app: Application, num: int):
    prsnt = pal_is_fru_prsnt(num)
    if prsnt is None or prsnt == 0:
        return

    base_route = "/api/server" + repr(num)
    node_server_shim = RestShim(get_node_server(num), base_route)

    app.router.add_get(node_server_shim.path, node_server_shim.get_handler)
    app.router.add_post(node_server_shim.path, node_server_shim.post_handler)

    # Add /api/server(1-n)/fruid
    fruid_path = os.path.join(base_route, "fruid")
    node_fruid_shim = RestShim(get_node_fruid("slot" + repr(num)), fruid_path)
    app.router.add_get(node_fruid_shim.path, node_fruid_shim.get_handler)
    # Add /api/server(1-n)/sensors
    sensors_path = os.path.join(base_route, "sensors")
    node_sensors_shim = RestShim(get_node_sensors("slot" + repr(num)), sensors_path)
    app.router.add_get(node_sensors_shim.path, node_sensors_shim.get_handler)
    app.router.add_post(node_sensors_shim.path, node_sensors_shim.post_handler)
    # Add /api/server(1-n)/logs
    logs_path = os.path.join(base_route, "logs")
    node_logs_shim = RestShim(get_node_logs("slot" + repr(num)), logs_path)
    app.router.add_get(node_logs_shim.path, node_logs_shim.get_handler)
    app.router.add_post(node_logs_shim.path, node_logs_shim.post_handler)
    # Add /api/server(1-n)/config
    config_path = os.path.join(base_route, "config")
    node_config_shim = RestShim(get_node_config("slot" + repr(num)), config_path)
    app.router.add_get(node_config_shim.path, node_config_shim.get_handler)
    app.router.add_post(node_config_shim.path, node_config_shim.post_handler)


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Add /api/spb to represent side plane board
    spb_shim = RestShim(get_node_spb(), "/api/spb")
    app.router.add_get(spb_shim.path, spb_shim.get_handler)
    app.router.add_post(spb_shim.path, spb_shim.post_handler)
    # Add /api/mezz to represent Network Mezzaine card
    mezz_shim = RestShim(get_node_mezz(), "/api/mezz")
    app.router.add_get(mezz_shim.path, mezz_shim.get_handler)

    # Add servers /api/server[1-max]
    num = pal_get_num_slots()
    for i in range(1, num + 1):
        populate_server_node(app, i)

    # Add /api/spb/fruid end point
    spb_fruid_shim = RestShim(get_node_fruid("spb"), "/api/spb/fruid")
    app.router.add_get(spb_fruid_shim.path, spb_fruid_shim.get_handler)
    # /api/spb/bmc end point
    spb_bmc_shim = RestShim(get_node_bmc(), "/api/spb/bmc")
    app.router.add_get(spb_bmc_shim.path, spb_bmc_shim.get_handler)
    app.router.add_post(spb_bmc_shim.path, spb_bmc_shim.post_handler)
    # /api/spb/sensors end point
    spb_sensors_shim = RestShim(get_node_sensors("spb"), "/api/spb/sensors")
    app.router.add_get(spb_sensors_shim.path, spb_sensors_shim.get_handler)
    app.router.add_post("/api/spb/sensors", spb_sensors_shim.post_handler)
    # Add /api/spb/fans end point
    spb_fans_shim = RestShim(get_node_fans(), "/api/spb/fans")
    app.router.add_get(spb_fans_shim.path, spb_fans_shim.get_handler)
    # /api/spb/logs end point
    spb_logs_shim = RestShim(get_node_logs("spb"), "/api/spb/logs")
    app.router.add_get(spb_logs_shim.path, spb_logs_shim.get_handler)
    app.router.add_post(spb_logs_shim.path, spb_logs_shim.post_handler)

    # Add /api/mezz/fruid end point
    nic_fruid_shim = RestShim(get_node_fruid("nic"), "/api/mezz/fruid")
    app.router.add_get(nic_fruid_shim.path, nic_fruid_shim.get_handler)
    # /api/mezz/sensors end point
    nic_sensors_shim = RestShim(get_node_sensors("nic"), "/api/mezz/sensors")
    app.router.add_get(nic_sensors_shim.path, nic_sensors_shim.get_handler)
    app.router.add_post(nic_sensors_shim.path, nic_sensors_shim.post_handler)
    # /api/mezz/logs end point
    nic_logs_shim = RestShim(get_node_logs("nic"), "/api/mezz/logs")
    app.router.add_get(nic_logs_shim.path, nic_logs_shim.get_handler)
    app.router.add_post(nic_logs_shim.path, nic_logs_shim.post_handler)
