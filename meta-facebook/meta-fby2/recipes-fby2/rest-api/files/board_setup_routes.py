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
from time import sleep

from aiohttp.web import Application
from compute_rest_shim import RestShim
from node_bios import (
    get_node_bios,
    get_node_bios_boot_order_trunk,
    get_node_bios_postcode_trunk,
    get_node_bios_plat_info_trunk,
    get_node_bios_boot_mode,
    get_node_bios_clear_cmos,
    get_node_bios_force_boot_setup,
    get_node_bios_boot_order,
)
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_device, get_node_server
from node_spb import get_node_spb
from rest_pal_legacy import pal_is_fru_prsnt, pal_get_num_slots


def get_slot_type(num):
    try:
        with open("/tmp/slot" + repr(num) + ".bin", "r") as f:
            retry = 3
            while retry != 0:
                value = f.read()
                if value:
                    slot_type = int(value.split()[0], 10)
                    break
                retry = retry - 1
                sleep(0.01)
                continue
    except Exception:
        slot_type = 3
    return slot_type


def populate_server_node(app: Application, num: int):
    prsnt = pal_is_fru_prsnt(num)
    if prsnt is None or prsnt == 0:
        return

    slot_type = get_slot_type(num)

    if slot_type == 0:
        base_route = "/api/server" + repr(num)
        node_server_shim = RestShim(get_node_server(num), base_route)
    else:
        base_route = "/api/device" + repr(num)
        node_server_shim = RestShim(get_node_device(num), base_route)

    app.router.add_get(base_route, node_server_shim.get_handler)
    app.router.add_post(base_route, node_server_shim.post_handler)

    # Add /api/server(1-n)/fruid
    fruid_path = os.path.join(base_route, "fruid")
    node_fruid_shim = RestShim(get_node_fruid("slot" + repr(num)), fruid_path)
    app.router.add_get(fruid_path, node_fruid_shim.get_handler)
    # Add /api/server(1-n)/sensors
    sensors_path = os.path.join(base_route, "sensors")
    node_sensors_shim = RestShim(get_node_sensors("slot" + repr(num)), sensors_path)
    app.router.add_get(sensors_path, node_sensors_shim.get_handler)
    app.router.add_post(sensors_path, node_sensors_shim.post_handler)
    # Add /api/server(1-n)/logs
    logs_path = os.path.join(base_route, "logs")
    node_logs_shim = RestShim(get_node_logs("slot" + repr(num)), logs_path)
    app.router.add_get(logs_path, node_logs_shim.get_handler)
    app.router.add_post(logs_path, node_logs_shim.post_handler)
    # Add /api/server(1-n)/config
    config_path = os.path.join(base_route, "config")
    node_config_shim = RestShim(get_node_config("slot" + repr(num)), config_path)
    app.router.add_get(config_path, node_config_shim.get_handler)
    app.router.add_post(config_path, node_config_shim.post_handler)

    if slot_type == 0:
        # Add /api/server(1-n)/bios
        bios_path = os.path.join(base_route, "bios")
        node_bios_shim = RestShim(get_node_bios("server" + repr(num)), bios_path)
        app.router.add_get(bios_path, node_bios_shim.get_handler)

        # Add /api/server(1-n)/bios/boot-order
        boot_order_trunk_path = os.path.join(base_route, "bios", "boot-order")
        boot_order_trunk_shim = RestShim(
            get_node_bios_boot_order_trunk("slot" + repr(num)),
            boot_order_trunk_path,
        )
        app.router.add_get(boot_order_trunk_path, boot_order_trunk_shim.get_handler)

        # Add /api/server(1-n)/bios/postcode
        postcode_path = os.path.join(base_route, "bios", "postcode")
        postcode_shim = RestShim(
            get_node_bios_postcode_trunk("slot" + repr(num)),
            postcode_path,
        )
        app.router.add_get(postcode_path, postcode_shim.get_handler)

        # Add /api/server(1-n)/bios/plat-info
        platinfo_path = os.path.join(base_route, "bios", "plat-info")
        platinfo_shim = RestShim(
            get_node_bios_plat_info_trunk("slot" + repr(num)),
            platinfo_path,
        )
        app.router.add_get(platinfo_path, platinfo_shim.get_handler)
        # Add /api/server(1-n)/bios/boot-order/boot_mode
        bootmode_path = os.path.join(base_route, "bios", "boot-order", "boot_mode")
        bootmode_shim = RestShim(
            get_node_bios_boot_mode("slot" + repr(num)),
            bootmode_path,
        )
        app.router.add_get(bootmode_path, bootmode_shim.get_handler)
        app.router.add_post(bootmode_path, bootmode_shim.post_handler)
        # Add /api/server(1-n)/bios/boot-order/clear_cmos
        clear_cmos_path = os.path.join(base_route, "bios", "boot-order", "clear_cmos")
        clear_cmos_shim = RestShim(
            get_node_bios_clear_cmos("slot" + repr(num)),
            clear_cmos_path,
        )
        app.router.add_get(clear_cmos_path, clear_cmos_shim.get_handler)
        app.router.add_post(clear_cmos_path, clear_cmos_shim.post_handler)
        # Add /api/server(1-n)/bios/boot-order/force_boot_bios_setup
        force_boot_bios_setup_path = os.path.join(
            base_route, "bios", "boot-order", "force_boot_bios_setup"
        )
        force_boot_bios_setup_shim = RestShim(
            get_node_bios_force_boot_setup("slot" + repr(num)),
            force_boot_bios_setup_path,
        )
        app.router.add_get(
            force_boot_bios_setup_path, force_boot_bios_setup_shim.get_handler
        )
        app.router.add_post(
            force_boot_bios_setup_path, force_boot_bios_setup_shim.post_handler
        )
        # Add /api/server(1-n)/bios/boot-order/boot_order
        boot_order_path = os.path.join(base_route, "bios", "boot-order", "boot_order")
        boot_order_shim = RestShim(
            get_node_bios_boot_order("slot" + repr(num)),
            boot_order_path,
        )
        app.router.add_get(boot_order_path, boot_order_shim.get_handler)
        app.router.add_post(boot_order_path, boot_order_shim.post_handler)


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Add /api/spb to represent side plane board
    spb_shim = RestShim(get_node_spb(), "/api/spb")
    app.router.add_get("/api/spb", spb_shim.get_handler)
    app.router.add_post("/api/spb", spb_shim.post_handler)
    # Add /api/mezz to represent Network Mezzaine card
    mezz_shim = RestShim(get_node_mezz(), "/api/mezz")
    app.router.add_get("/api/mezz", mezz_shim.get_handler)

    # Add servers /api/server[1-max]
    num = pal_get_num_slots()
    for i in range(1, num + 1):
        populate_server_node(app, i)

    # Add /api/spb/fruid end point
    spb_fruid_shim = RestShim(get_node_fruid("spb"), "/api/spb/fruid")
    app.router.add_get("/api/spb/fruid", spb_fruid_shim.get_handler)
    # /api/spb/bmc end point
    spb_bmc_shim = RestShim(get_node_bmc(), "/api/spb/bmc")
    app.router.add_get("/api/spb/bmc", spb_bmc_shim.get_handler)
    app.router.add_post("/api/spb/bmc", spb_bmc_shim.post_handler)
    # /api/spb/sensors end point
    spb_sensors_shim = RestShim(get_node_sensors("spb"), "/api/spb/sensors")
    app.router.add_get("/api/spb/sensors", spb_sensors_shim.get_handler)
    app.router.add_post("/api/spb/sensors", spb_sensors_shim.post_handler)
    # Add /api/spb/fans end point
    spb_fans_shim = RestShim(get_node_fans(), "/api/spb/fans")
    app.router.add_get("/api/spb/fans", spb_fans_shim.get_handler)
    # /api/spb/logs end point
    spb_logs_shim = RestShim(get_node_logs("spb"), "/api/spb/logs")
    app.router.add_get("/api/spb/logs", spb_logs_shim.get_handler)
    app.router.add_post("/api/spb/logs", spb_logs_shim.post_handler)

    # Add /api/mezz/fruid end point
    nic_fruid_shim = RestShim(get_node_fruid("nic"), "/api/mezz/fruid")
    app.router.add_get("/api/mezz/fruid", nic_fruid_shim.get_handler)
    # /api/mezz/sensors end point
    nic_sensors_shim = RestShim(get_node_sensors("nic"), "/api/mezz/sensors")
    app.router.add_get("/api/mezz/sensors", nic_sensors_shim.get_handler)
    app.router.add_post("/api/mezz/sensors", nic_sensors_shim.post_handler)
    # /api/mezz/logs end point
    nic_logs_shim = RestShim(get_node_logs("nic"), "/api/mezz/logs")
    app.router.add_get("/api/mezz/logs", nic_logs_shim.get_handler)
    app.router.add_post("/api/mezz/logs", nic_logs_shim.post_handler)
