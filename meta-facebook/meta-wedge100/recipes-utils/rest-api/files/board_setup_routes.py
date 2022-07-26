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
from board_endpoint import boardApp_Handler
from common_endpoint import commonApp_Handler
from boardroutes import board_routes
from rest_utils import common_routes

from aiohttp.web import Application


def setup_board_routes(app: Application, write_enabed: bool):
    bhandler = boardApp_Handler()
    chandler = commonApp_Handler()
    # app.router.add_get(board_routes[0], bhandler.rest_usb2i2c_reset_hdl)
    app.router.add_get(board_routes[0], bhandler.rest_i2cflush_hdl)
    app.router.add_get(board_routes[1], bhandler.rest_firmware_info_hdl)
    app.router.add_get(board_routes[2], bhandler.rest_firmware_info_all_hdl)
    app.router.add_get(board_routes[3], bhandler.rest_firmware_info_fan_hdl)
    app.router.add_get(board_routes[4], bhandler.rest_firmware_info_sys_hdl)
    app.router.add_get(
        board_routes[5], bhandler.rest_firmware_info_internal_switch_config_hdl
    )
    app.router.add_get(board_routes[6], bhandler.rest_presence_hdl)
    app.router.add_get(board_routes[7], bhandler.rest_presence_pem_hdl)
    app.router.add_get(board_routes[8], bhandler.rest_presence_psu_hdl)

    # TOR endpoints
    app.router.add_get(common_routes[11], chandler.rest_modbus_get)
    app.router.add_post(common_routes[12], chandler.rest_modbus_cmd_post)
    app.router.add_get(common_routes[13], chandler.rest_modbus_registers_get)
