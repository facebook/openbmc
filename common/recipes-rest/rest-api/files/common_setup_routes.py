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
import node
from aiohttp.log import server_logger
from aiohttp.web import Application
from redfish_common_routes import Redfish
from rest_ntpstatus import rest_ntp_status_handler

try:
    from compute_rest_shim import RestShim
    from node_api import get_node_api
    from node_attestation import setup_attestation_endpoints
    from node_bmc import get_node_bmc
    from node_fans import get_node_fans
    from node_logs import get_node_logs
    from node_sensors import get_node_sensors

    compute = True
except ImportError:
    compute = False
    from common_endpoint import commonApp_Handler
    from rest_utils import common_routes


def setup_rackmon_routes(app: Application):
    chandler = commonApp_Handler()
    # TOR endpoints
    app.router.add_get(common_routes[11], chandler.rest_modbus_get)
    app.router.add_post(common_routes[12], chandler.rest_modbus_cmd_post)
    app.router.add_get(common_routes[13], chandler.rest_modbus_registers_get)


def setup_common_routes(app: Application, write_enabled: bool):
    if compute:
        server_logger.info("Adding compute base-routes")
        # compute specific common routes
        # Create /api end point as root node
        node_shim = RestShim(get_node_api(), "/api")
        app.router.add_get(node_shim.path, node_shim.get_handler)

        # /api/bmc end point
        bmc_shim = RestShim(get_node_bmc(), "/api/bmc")
        app.router.add_get(bmc_shim.path, bmc_shim.get_handler)
        app.router.add_post(bmc_shim.path, bmc_shim.post_handler)

        # /api/sensors end point
        sensors_shim = RestShim(get_node_sensors("all"), "/api/sensors")
        app.router.add_get(sensors_shim.path, sensors_shim.get_handler)
        app.router.add_post(sensors_shim.path, sensors_shim.post_handler)

        # /api/logs end point
        logs_shim = RestShim(get_node_logs("all"), "/api/logs")
        app.router.add_get(logs_shim.path, logs_shim.get_handler)
        app.router.add_post(logs_shim.path, logs_shim.post_handler)

        # /api/fans end point
        fans_shim = RestShim(get_node_fans(), "/api/fans")
        app.router.add_get(fans_shim.path, fans_shim.get_handler)
        sys_shim = RestShim(node.node(), "/api/sys")
        app.router.add_get(sys_shim.path, sys_shim.get_handler)

        # /attestation endpoints
        setup_attestation_endpoints(app)

    else:
        # FBOSS specific common routes
        server_logger.info("Adding FBOSS base-routes")
        chandler = commonApp_Handler()
        app.router.add_get(common_routes[0], chandler.rest_api)
        app.router.add_get(common_routes[1], chandler.rest_sys)
        app.router.add_get(common_routes[2], chandler.rest_mb_sys)
        app.router.add_get(common_routes[3], chandler.rest_fruid_hdl)
        app.router.add_get(common_routes[4], chandler.rest_bmc_hdl)
        app.router.add_get(common_routes[5], chandler.rest_server_hdl)
        app.router.add_post(common_routes[5], chandler.rest_server_act_hdl)
        app.router.add_get(common_routes[6], chandler.rest_sensors_hdl)
        app.router.add_get(common_routes[7], chandler.rest_gpios_hdl)
        app.router.add_get(common_routes[8], chandler.rest_fcpresent_hdl)
        app.router.add_get(common_routes[9], chandler.psu_update_hdl)
        app.router.add_post(common_routes[9], chandler.psu_update_hdl_post)
        app.router.add_post(common_routes[10], chandler.rest_fscd_sensor_data_post)
    # common routes for all openbmc.
    server_logger.info("Adding common routes")
    app.router.add_get("/api/sys/ntp", rest_ntp_status_handler)
    server_logger.info("Adding Redfish common routes")
    redfish = Redfish()
    redfish.setup_redfish_common_routes(app)
