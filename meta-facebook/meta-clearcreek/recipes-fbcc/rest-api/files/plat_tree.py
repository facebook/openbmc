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
from node_api import get_node_api
from node_bmc import get_node_bmc
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_sensors import get_node_sensors
from node_server import get_node_server_2s
from node_sled import get_node_sled
from tree import tree

from aiohttp.web import Application


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Create /api end point as root node
    r_api = tree("api", data=get_node_api())

    # Add /api/sled to represent entire SLED
    r_sled = tree("sled", data=get_node_sled())
    r_api.addChild(r_sled)

    # Add mb /api/sled/mb
    r_mb = tree("mb", data=get_node_server_2s(1, "mb"))
    r_sled.addChild(r_mb)

    # Add /api/sled/mb/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("mb"))
    r_mb.addChild(r_temp)

    # /api/sled/mb/bmc end point
    r_temp = tree("bmc", data=get_node_bmc())
    r_mb.addChild(r_temp)

    # /api/sled/mb/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("mb"))
    r_mb.addChild(r_temp)

    # /api/sled/mb/logs end point
    r_temp = tree("logs", data=get_node_logs("mb"))
    r_mb.addChild(r_temp)

    r_api.setup(app, write_enabled)
