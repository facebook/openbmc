#!/usr/bin/env python
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
from node_enclosure import *
from node_bios import *
from node_health import get_node_health
from node_identify import get_node_identify
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_device, get_node_server
from node_uic import get_node_uic
from node_dpb import get_node_dpb
from node_scc import get_node_scc
from node_e1s_iocm import get_node_e1s_iocm
from rest_pal_legacy import *
from tree import tree

from aiohttp.web import Application

def populate_server_node(r_api, num):
    prsnt = pal_is_fru_prsnt(num)
    if prsnt == None or prsnt == 0:
        return None

    r_server = tree("server", data=get_node_server(num))
    r_api.addChild(r_server)
    
    # Add server/fruid, sensors, logs, config, bios end points
    r_fruid = tree("fruid", data=get_node_fruid("server"))
    r_sensors = tree("sensors", data=get_node_sensors("server"))
    r_logs = tree("logs", data=get_node_logs("server"))
    r_config = tree("config", data=get_node_config("server"))
    r_bios = tree("bios", data=get_node_bios("server"))
    r_server.addChildren([r_fruid, r_sensors, r_logs, r_config, r_bios])

    # Add server/bios/boot-order, postcode, plat-info end points
    r_boot_order_trunk = tree(
        "boot-order", data=get_node_bios_boot_order_trunk("server")
    )
    r_postcode_trunk = tree(
        "postcode", data=get_node_bios_postcode_trunk("server")
    )
    r_plat_info_trunk = tree(
        "plat-info", data=get_node_bios_plat_info_trunk("server")
    )
    r_bios.addChildren([r_boot_order_trunk, r_postcode_trunk, r_plat_info_trunk])

    # Add server/bios/boot-order/boot_mode, clear_cmos, force_boot_bios_setup, boot_order end points
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

def populate_e1s_iocm_node(r_api):
    FRU_E1S_IOCM = 7
    is_supported_iocm = False

    cap = pal_get_fru_capability(FRU_E1S_IOCM)

    if (cap & FRU_CAPABILITY_FRUID_ALL) > 0:
        is_supported_iocm = True

    r_e1s_iocm = tree("e1s_iocm", data=get_node_e1s_iocm())
    r_api.addChild(r_e1s_iocm)

    # Add /api/e1s_iocm/fruid end point
    if is_supported_iocm == True:
        r_temp = tree("fruid", data=get_node_fruid("e1s_iocm"))
        r_e1s_iocm.addChild(r_temp)
    # /api/e1s_iocm/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("e1s_iocm"))
    r_e1s_iocm.addChild(r_temp)
    # /api/e1s_iocm/logs end point
    r_temp = tree("logs", data=get_node_logs("e1s_iocm"))
    r_e1s_iocm.addChild(r_temp)

    return r_e1s_iocm

# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Create /api end point as root node
    r_api = tree("api", data=get_node_api())

    # Add /api/nic to represent NIC card
    r_nic = tree("nic", data=get_node_mezz())
    r_api.addChild(r_nic)

    # Add servers /api/server[1-max]
    num = pal_get_num_slots()
    for i in range(1, num + 1):
        r_server = populate_server_node(r_api, i)

    # Add /api/nic/fruid end point
    r_temp = tree("fruid", data=get_node_fruid("nic"))
    r_nic.addChild(r_temp)
    # /api/nic/sensors end point
    r_temp = tree("sensors", data=get_node_sensors("nic"))
    r_nic.addChild(r_temp)
    # /api/nic/logs end point
    r_temp = tree("logs", data=get_node_logs("nic"))
    r_nic.addChild(r_temp)

    # Add /api/uic to represent User Interface Card
    r_uic = tree("uic", data=get_node_uic())
    r_api.addChild(r_uic)

    # Add /api/uic/fruid end point
    r_fruid = tree("fruid", data=get_node_fruid("uic"))
    # /api/uic/sensors end point
    r_sensors = tree("sensors", data=get_node_sensors("uic"))
    # /api/uic/logs end point
    r_logs = tree("logs", data=get_node_logs("uic"))
    # /api/uic/bmc end point
    r_bmc = tree("bmc", data=get_node_bmc())
    # /api/uic/health end point
    r_health = tree("health", data=get_node_health())
    # /api/uic/identify end point
    r_identify = tree("identify", data=get_node_identify("uic"))
    r_uic.addChildren([r_fruid, r_sensors, r_logs, r_bmc, r_health, r_identify])

    # Add /api/dpb to represent Drive Plan Board
    r_dpb = tree("dpb", data=get_node_dpb())
    r_api.addChild(r_dpb)

    # Add /api/dpb/fruid end point
    r_fruid = tree("fruid", data=get_node_fruid("dpb"))
    # /api/dpb/sensors end point
    r_sensors = tree("sensors", data=get_node_sensors("dpb"))
    # /api/dpb/logs end point
    r_logs = tree("logs", data=get_node_logs("dpb"))
    # /api/dpb/fans end point
    r_fans = tree("fans", data=get_node_fans())
    # /api/dpb/error end point
    r_error = tree("error", data=get_node_enclosure_error())
    # /api/dpb/hdd-status end point
    r_hdd_status = tree("hdd-status", data=get_node_enclosure_hdd_status())
    r_dpb.addChildren([r_fruid, r_sensors, r_logs, r_fans, r_error, r_hdd_status])

    # Add /api/scc to represent Storage Controller Card
    r_scc = tree("scc", data=get_node_scc())
    r_api.addChild(r_scc)

    # Add /api/scc/fruid end point
    r_fruid = tree("fruid", data=get_node_fruid("scc"))
    # /api/scc/sensors end point
    r_sensors = tree("sensors", data=get_node_sensors("scc"))
    # /api/scc/logs end point
    r_logs = tree("logs", data=get_node_logs("scc"))
    r_scc.addChildren([r_fruid, r_sensors, r_logs])

    # Add /api/e1s_iocm to represent E1.S or IOC Module
    r_e1s_iocm = populate_e1s_iocm_node(r_api)

    r_api.setup(app, write_enabled)

