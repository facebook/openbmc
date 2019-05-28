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
from node_bios import *
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_dpb import get_node_dpb
from node_enclosure import *
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_health import get_node_health
from node_identify import get_node_identify
from node_iom import get_node_iom
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_scc import get_node_scc
from node_sensors import get_node_sensors
from node_server import get_node_server
from pal import *
from tree import tree


def populate_server_node(num):
    prsnt = pal_is_fru_prsnt(num)
    if prsnt == None or prsnt == 0:
        return None

    r_server = tree("server", data=get_node_server(num))
    r_fruid = tree("fruid", data=get_node_fruid("server"))
    r_sensors = tree("sensors", data=get_node_sensors("server"))
    r_logs = tree("logs", data=get_node_logs("server"))
    r_config = tree("config", data=get_node_config("server"))
    r_bios = tree("bios", data=get_node_bios("server"))
    r_server.addChildren([r_fruid, r_sensors, r_logs, r_config, r_bios])

    r_boot_order_trunk = tree(
        "boot-order", data=get_node_bios_boot_order_trunk("server")
    )
    r_postcode_trunk = tree("postcode", data=get_node_bios_postcode_trunk("server"))
    r_plat_info_trunk = tree("plat-info", data=get_node_bios_plat_info_trunk("server"))
    r_pcie_port_config_trunk = tree(
        "pcie-port-config", data=get_node_bios_pcie_port_config_trunk("server")
    )
    r_bios.addChildren(
        [
            r_boot_order_trunk,
            r_postcode_trunk,
            r_plat_info_trunk,
            r_pcie_port_config_trunk,
        ]
    )

    r_boot_mode = tree("boot_mode", data=get_node_bios_boot_mode("server"))
    r_clear_cmos = tree("clear_cmos", data=get_node_bios_clear_cmos("server"))
    r_force_boot_bios_setup = tree(
        "force_boot_bios_setup", data=get_node_bios_force_boot_setup("server")
    )
    r_boot_order = tree("boot_order", data=get_node_bios_boot_order("server"))
    r_boot_order_trunk.addChildren(
        [r_boot_mode, r_clear_cmos, r_force_boot_bios_setup, r_boot_order]
    )

    return r_server


# Initialize Platform specific Resource Tree
def init_plat_tree():

    # Create /api end point as root node
    r_api = tree("api", data=get_node_api())

    # Add /api/mezz to represent Mezzanine Card
    r_mezz = tree("nic", data=get_node_mezz())
    r_api.addChild(r_mezz)
    # Add /api/iom to represent IO Module
    r_iom = tree("iom", data=get_node_iom())
    r_api.addChild(r_iom)
    # Add /api/dpb to represent Drive Plan Board
    r_dpb = tree("dpb", data=get_node_dpb())
    r_api.addChild(r_dpb)
    # Add /api/scc to represent Storage Controller Card
    r_scc = tree("scc", data=get_node_scc())
    r_api.addChild(r_scc)

    # Add servers /api/slot[1-max]
    num = pal_get_num_slots()
    for i in range(1, num + 1):
        r_server = populate_server_node(i)
        if r_server:
            r_api.addChild(r_server)

    # Add /api/mezz/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("nic"))
    r_mezz.addChild(r_temp)
    # Add /api/mezz/logs end point
    r_temp = tree("logs", data=get_node_logs("nic"))
    r_mezz.addChild(r_temp)

    # Add /api/iom/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("iom"))
    r_iom.addChild(r_temp)
    # Add /api/iom/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("iom"))
    r_iom.addChild(r_temp)
    # Add /api/iom/logs end point
    r_temp = tree("logs", data=get_node_logs("iom"))
    r_iom.addChild(r_temp)
    # Add /api/iom/bmc end point
    r_temp = tree("bmc", data=get_node_bmc())
    r_iom.addChild(r_temp)
    # Add /api/iom/health end point
    r_temp = tree("health", data=get_node_health())
    r_iom.addChild(r_temp)
    # Add /api/iom/identify end point
    r_temp = tree("identify", data=get_node_identify("iom"))
    r_iom.addChild(r_temp)

    # Add /api/dpb/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("dpb"))
    r_dpb.addChild(r_temp)
    # Add /api/dpb/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("dpb"))
    r_dpb.addChild(r_temp)
    # Add /api/dpb/logs end point
    r_temp = tree("logs", data=get_node_logs("dpb"))
    r_dpb.addChild(r_temp)
    # Add /api/dpb/fans end point
    r_temp = tree("fans", data=get_node_fans())
    r_dpb.addChild(r_temp)
    # Add /api/dpb/hdd-status end point
    r_tmp = tree("hdd-status", data=get_node_enclosure_hdd_status())
    r_dpb.addChild(r_tmp)
    # Add /api/dpb/error end point
    r_tmp = tree("error", data=get_node_enclosure_error())
    r_dpb.addChild(r_tmp)
    # Add /api/dpb/flash-health end point
    r_tmp = tree("flash-health", data=get_node_enclosure_flash_health())
    r_dpb.addChild(r_tmp)
    # Add /api/dpb/flash-status end point
    r_tmp = tree("flash-status", data=get_node_enclosure_flash_status())
    r_dpb.addChild(r_tmp)

    # Add /api/scc/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("scc"))
    r_scc.addChild(r_temp)
    # Add /api/scc/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("scc"))
    r_scc.addChild(r_temp)
    # Add /api/scc/logs end point
    r_temp = tree("logs", data=get_node_logs("scc"))
    r_scc.addChild(r_temp)

    return r_api
