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
from common_endpoint import commonApp_Handler
from rest_utils import *

def setup_common_routes(app):
    chandler = commonApp_Handler()
    app.router.add_get(common_routes[0],chandler.rest_api)
    app.router.add_get(common_routes[1], chandler.rest_sys)
    app.router.add_get(common_routes[2], chandler.rest_mb_sys)
    app.router.add_get(common_routes[3], chandler.rest_fruid_hdl)
    app.router.add_get(common_routes[4], chandler.rest_bmc_hdl)
    app.router.add_get(common_routes[5], chandler.rest_server_hdl)
    app.router.add_post(common_routes[5], chandler.rest_server_act_hdl)
    app.router.add_get(common_routes[6], chandler.rest_sensors_hdl)
    app.router.add_get(common_routes[7], chandler.rest_gpios_hdl)
    app.router.add_get(common_routes[8], chandler.rest_fcpresent_hdl)
    app.router.add_get(common_routes[9], chandler.modbus_registers_hdl)
    app.router.add_get(common_routes[10], chandler.psu_update_hdl)
    app.router.add_post(common_routes[10], chandler.psu_update_hdl_post)
    app.router.add_get(common_routes[11], chandler.rest_slotid_hdl)
    app.router.add_get(common_routes[12], chandler.rest_mTerm_status)
