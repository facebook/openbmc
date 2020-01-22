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
from time import sleep

from node_api import get_node_api
from node_bios import *
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_device, get_node_server
from node_spb import get_node_spb
from pal import *
from tree import tree

def populate_server_node(r_api, num):
    prsnt = pal_is_fru_prsnt(num)
    if prsnt == None or prsnt == 0:
        return None

    r_server = tree("server" + repr(num), data=get_node_server(num))

    r_api.addChild(r_server)
    r_fruid = tree("fruid", data=get_node_fruid("slot" + repr(num)))
    r_sensors = tree("sensors", data=get_node_sensors("slot" + repr(num)))
    r_logs = tree("logs", data=get_node_logs("slot" + repr(num)))
    r_config = tree("config", data=get_node_config("slot" + repr(num)))
    r_bios = tree("bios", data=get_node_bios("server" + repr(num)))
    r_server.addChildren([r_fruid, r_sensors, r_logs, r_config, r_bios])

    r_boot_order_trunk = tree(
        "boot-order", data=get_node_bios_boot_order_trunk("slot" + repr(num))
    )
    r_postcode_trunk = tree(
        "postcode", data=get_node_bios_postcode_trunk("slot" + repr(num))
    )
    r_plat_info_trunk = tree(
        "plat-info", data=get_node_bios_plat_info_trunk("slot" + repr(num))
    )
    r_bios.addChildren([r_boot_order_trunk, r_postcode_trunk, r_plat_info_trunk])

    r_boot_mode = tree("boot_mode", data=get_node_bios_boot_mode("slot" + repr(num)))
    r_clear_cmos = tree("clear_cmos", data=get_node_bios_clear_cmos("slot" + repr(num)))
    r_force_boot_bios_setup = tree(
        "force_boot_bios_setup", data=get_node_bios_force_boot_setup("slot" + repr(num))
    )
    r_boot_order = tree("boot_order", data=get_node_bios_boot_order("slot" + repr(num)))
    r_boot_order_trunk.addChildren(
        [r_boot_mode, r_clear_cmos, r_force_boot_bios_setup, r_boot_order]
    )

    return r_server


# Initialize Platform specific Resource Tree
def init_plat_tree():

    # Create /api end point as root node
    r_api = tree("api", data=get_node_api())

    # Add /api/spb to represent side plane board
    r_spb = tree("spb", data=get_node_spb())
    r_api.addChild(r_spb)

    # Add /api/mezz to represent Network Mezzaine card
    r_mezz = tree("mezz", data=get_node_mezz())
    r_api.addChild(r_mezz)

    # Add servers /api/server[1-max]
    num = pal_get_num_slots()
    for i in range(1, num + 1):
        r_server = populate_server_node(r_api, i)

    # Add /api/spb/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("spb"))
    r_spb.addChild(r_temp)
    # /api/spb/bmc end point
    r_temp = tree("bmc", data=get_node_bmc())
    r_spb.addChild(r_temp)
    # /api/spb/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("spb"))
    r_spb.addChild(r_temp)
    # Add /api/spb/fans end point
    r_temp = tree("fans", data=get_node_fans())
    r_spb.addChild(r_temp)
    # /api/spb/logs end point
    r_temp = tree("logs", data=get_node_logs("spb"))
    r_spb.addChild(r_temp)

    # Add /api/mezz/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("nic"))
    r_mezz.addChild(r_temp)
    # /api/mezz/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("nic"))
    r_mezz.addChild(r_temp)
    # /api/mezz/logs end point
    r_temp = tree("logs", data=get_node_logs("nic"))
    r_mezz.addChild(r_temp)

    return r_api
