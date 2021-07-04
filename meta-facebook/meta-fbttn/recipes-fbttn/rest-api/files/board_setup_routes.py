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
from node_bios import (
    get_node_bios,
    get_node_bios_boot_order_trunk,
    get_node_bios_postcode_trunk,
    get_node_bios_plat_info_trunk,
    get_node_bios_pcie_port_config_trunk,
    get_node_bios_boot_mode,
    get_node_bios_clear_cmos,
    get_node_bios_force_boot_setup,
    get_node_bios_boot_order,
)
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_dpb import get_node_dpb
from node_enclosure import (
    get_node_enclosure_hdd_status,
    get_node_enclosure_flash_status,
    get_node_enclosure_error,
    get_node_enclosure_flash_health,
)
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


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Add /api/nic to represent Mezzanine Card
    mezz_shim = RestShim(get_node_mezz(), "/api/nic")
    app.router.add_get(mezz_shim.path, mezz_shim.get_handler)

    # Add /api/iom to represent IO Module
    iom_shim = RestShim(get_node_iom(), "/api/iom")
    app.router.add_get(iom_shim.path, iom_shim.get_handler)
    # Add /api/dpb to represent Drive Plan Board
    dpb_shim = RestShim(get_node_dpb(), "/api/dpb")
    app.router.add_get(dpb_shim.path, dpb_shim.get_handler)
    # Add /api/scc to represent Storage Controller Card
    scc_shim = RestShim(get_node_scc(), "/api/scc")
    app.router.add_get(scc_shim.path, scc_shim.get_handler)

    # Add /api/nic/sensors end point
    nic_sensors_shim = RestShim(get_node_sensors("nic"), "/api/nic/sensors")
    app.router.add_get(nic_sensors_shim.path, nic_sensors_shim.get_handler)
    app.router.add_post(nic_sensors_shim.path, nic_sensors_shim.post_handler)
    # Add /api/nic/logs end point
    nic_logs_shim = RestShim(get_node_logs("nic"), "/api/nic/logs")
    app.router.add_get(nic_logs_shim.path, nic_logs_shim.get_handler)
    app.router.add_post(nic_logs_shim.path, nic_logs_shim.post_handler)
    # Add /api/iom/fruid end point
    iom_fruid_shim = RestShim(get_node_fruid("iom"), "/api/iom/fruid")
    app.router.add_get(iom_fruid_shim.path, iom_fruid_shim.get_handler)
    # Add /api/iom/sensors end point
    iom_sensors_shim = RestShim(get_node_sensors("iom"), "/api/iom/sensors")
    app.router.add_get(iom_sensors_shim.path, iom_sensors_shim.get_handler)
    app.router.add_post(iom_sensors_shim.path, iom_sensors_shim.post_handler)
    # Add /api/iom/logs end point
    iom_logs_shim = RestShim(get_node_logs("iom"), "/api/iom/logs")
    app.router.add_get(iom_logs_shim.path, iom_logs_shim.get_handler)
    app.router.add_post(iom_logs_shim.path, iom_logs_shim.post_handler)
    # Add /api/iom/bmc end point
    iom_bmc_shim = RestShim(get_node_bmc(), "/api/iom/bmc")
    app.router.add_get(iom_bmc_shim.path, iom_bmc_shim.get_handler)
    app.router.add_post(iom_bmc_shim.path, iom_bmc_shim.post_handler)
    # Add /api/iom/health end point
    iom_health_shim = RestShim(get_node_health(), "/api/iom/health")
    app.router.add_get(iom_health_shim.path, iom_health_shim.get_handler)
    # Add /api/iom/identify end point
    iom_identify_shim = RestShim(get_node_identify("iom"), "/api/iom/identify")
    app.router.add_get(iom_identify_shim.path, iom_identify_shim.get_handler)
    app.router.add_post(iom_identify_shim.path, iom_identify_shim.post_handler)

    # Add /api/dpb/fruid end point
    dpb_fruid_shim = RestShim(get_node_fruid("dpb"), "/api/dpb/fruid")
    app.router.add_get(dpb_fruid_shim.path, dpb_fruid_shim.get_handler)
    # Add /api/dpb/sensors end point
    dpb_sensors_shim = RestShim(get_node_sensors("dpb"), "/api/dpb/sensors")
    app.router.add_get(dpb_sensors_shim.path, dpb_sensors_shim.get_handler)
    app.router.add_post(dpb_sensors_shim.path, dpb_sensors_shim.post_handler)
    # Add /api/dpb/logs end point
    dpb_logs_shim = RestShim(get_node_logs("dpb"), "/api/dpb/logs")
    app.router.add_get(dpb_logs_shim.path, dpb_logs_shim.get_handler)
    app.router.add_post(dpb_logs_shim.path, dpb_logs_shim.post_handler)

    # Add /api/dpb/fans end point
    dpb_fans_shim = RestShim(get_node_fans(), "/api/dpb/fans")
    app.router.add_get(dpb_fans_shim.path, dpb_fans_shim.get_handler)

    # Add /api/dpb/hdd-status end point
    dpb_hdd_shim = RestShim(get_node_enclosure_hdd_status(), "/api/dpb/hdd-status")
    app.router.add_get(dpb_hdd_shim.path, dpb_hdd_shim.get_handler)
    app.router.add_get(dpb_hdd_shim.path, dpb_hdd_shim.post_handler)

    # Add /api/dpb/error end point
    dpb_error_shim = RestShim(get_node_enclosure_error(), "/api/dpb/error")
    app.router.add_get(dpb_error_shim.path, dpb_error_shim.get_handler)
    # Add /api/dpb/flash-health end point
    dpb_flash_health_shim = RestShim(
        get_node_enclosure_flash_health(), "/api/dpb/flash-health"
    )
    app.router.add_get(dpb_flash_health_shim.path, dpb_flash_health_shim.get_handler)
    # Add /api/dpb/flash-status end point
    dpb_flash_status_shim = RestShim(
        get_node_enclosure_flash_status(), "/api/dpb/flash-status"
    )
    app.router.add_get(dpb_flash_status_shim.path, dpb_flash_status_shim.get_handler)

    # Add /api/scc/fruid end point
    scc_fruid_shim = RestShim(get_node_fruid("scc"), "/api/scc/fruid")
    app.router.add_get(scc_fruid_shim.path, scc_fruid_shim.get_handler)
    # Add /api/scc/sensors end point
    scc_sensors_shim = RestShim(get_node_sensors("scc"), "/api/scc/sensors")
    app.router.add_get(scc_sensors_shim.path, scc_sensors_shim.get_handler)
    app.router.add_post(scc_sensors_shim.path, scc_sensors_shim.post_handler)
    # Add /api/scc/logs end point
    scc_logs_shim = RestShim(get_node_logs("scc"), "/api/scc/logs")
    app.router.add_get(scc_logs_shim.path, scc_logs_shim.get_handler)
    app.router.add_post(scc_logs_shim.path, scc_logs_shim.post_handler)

    # Add /api/server node
    server_shim = RestShim(get_node_server(1), "/api/server")
    app.router.add_get(server_shim.path, server_shim.get_handler)
    app.router.add_post(server_shim.path, server_shim.post_handler)
    # Add /api/server/fruid node
    server_fruid_shim = RestShim(get_node_fruid("server"), "/api/server/fruid")
    app.router.add_get(server_fruid_shim.path, server_fruid_shim.get_handler)
    # Add /api/server/sensors node
    server_sensor_shim = RestShim(get_node_sensors("server"), "/api/server/sensors")
    app.router.add_get(server_sensor_shim.path, server_sensor_shim.get_handler)
    app.router.add_post(server_sensor_shim.path, server_sensor_shim.post_handler)
    # Add /api/server/logs node
    server_logs_shim = RestShim(get_node_logs("server"), "/api/server/logs")
    app.router.add_get(server_logs_shim.path, server_logs_shim.get_handler)
    app.router.add_post(server_logs_shim.path, server_logs_shim.post_handler)
    # Add /api/server/config node
    server_config_shim = RestShim(get_node_config("server"), "/api/server/config")
    app.router.add_get(server_config_shim.path, server_config_shim.get_handler)
    app.router.add_post(server_config_shim.path, server_config_shim.post_handler)
    # Add /api/server/bios node
    server_bios_shim = RestShim(get_node_bios("server"), "/api/server/bios")
    app.router.add_get(server_bios_shim.path, server_bios_shim.get_handler)
    # Add /api/server/bios/boot-order node
    boot_order_trunk_shim = RestShim(
        get_node_bios_boot_order_trunk("server"),
        "/api/server/bios/boot-order",
    )
    app.router.add_get(boot_order_trunk_shim.path, boot_order_trunk_shim.get_handler)
    # Add /api/server/bios/postcode node
    postcode_shim = RestShim(
        get_node_bios_postcode_trunk("server"),
        "/api/server/bios/postcode",
    )
    app.router.add_get(postcode_shim.path, postcode_shim.get_handler)
    # Add /api/server/bios/plat-info node
    plat_info_shim = RestShim(
        get_node_bios_plat_info_trunk("server"),
        "/api/server/bios/plat-info",
    )
    app.router.add_get(plat_info_shim.path, plat_info_shim.get_handler)
    # Add /api/server/bios/pcie-port-config node
    pcie_port_config_shim = RestShim(
        get_node_bios_pcie_port_config_trunk("server"),
        "/api/server/bios/pcie-port-config",
    )
    app.router.add_get(pcie_port_config_shim.path, pcie_port_config_shim.get_handler)
    app.router.add_post(pcie_port_config_shim.path, pcie_port_config_shim.post_handler)
    # Add /api/server/bios/boot-order/boot_mode node
    boot_mode_shim = RestShim(
        get_node_bios_boot_mode("server"),
        "/api/server/bios/boot-order/boot_mode",
    )
    app.router.add_get(boot_mode_shim.path, boot_mode_shim.get_handler)
    app.router.add_post(boot_mode_shim.path, boot_mode_shim.post_handler)
    # Add /api/server/bios/boot-order/clear_cmos node
    bios_clear_shim = RestShim(
        get_node_bios_clear_cmos("server"),
        "/api/server/bios/boot-order/clear_cmos",
    )
    app.router.add_get(bios_clear_shim.path, bios_clear_shim.get_handler)
    app.router.add_post(bios_clear_shim.path, bios_clear_shim.post_handler)
    # Add /api/server/bios/boot-order/force_boot_bios_setup node
    force_boot_shim = RestShim(
        get_node_bios_force_boot_setup("server"),
        "/api/server/bios/boot-order/force_boot_bios_setup",
    )
    app.router.add_get(force_boot_shim.path, force_boot_shim.get_handler)
    app.router.add_post(
        force_boot_shim.path,
        force_boot_shim.post_handler,
    )
    # Add /api/server/bios/boot-order/boot_order node
    boot_order_node_shim = RestShim(
        get_node_bios_boot_order("server"),
        "/api/server/bios/boot-order/boot_order",
    )
    app.router.add_get(boot_order_node_shim.path, boot_order_node_shim.get_handler)
    app.router.add_post(boot_order_node_shim.path, boot_order_node_shim.post_handler)
