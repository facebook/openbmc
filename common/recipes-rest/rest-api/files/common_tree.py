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
import os
import socket
import ssl
from ctypes import *

from node_api import get_node_api
from node_bmc import get_node_bmc
from node_fans import get_node_fans
from node_logs import get_node_logs
from node_sensors import get_node_sensors
from node_sled import get_node_sled
from node_attestation import get_tree_attestation
from tree import tree
from aiohttp.web import Application


def setup_common_routes(app: Application, write_enabled: bool):

    # Create /api end point as root node
    r_api = tree("api", data=get_node_api())

    # /api/bmc end point
    r_temp = tree("bmc", data=get_node_bmc())
    r_api.addChild(r_temp)

    # /api/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("all"))
    r_api.addChild(r_temp)

    # /api/logs end point
    r_temp = tree("logs", data=get_node_logs("all"))
    r_api.addChild(r_temp)

    # /api/fans end point
    r_temp = tree("fans", data=get_node_fans())
    r_api.addChild(r_temp)

    # /attestation endpoints
    r_api.addChild(get_tree_attestation())
    r_api.setup(app, write_enabled)
