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
from board_endpoint import boardApp_Handler
from boardroutes import *


def setup_board_routes(app):
    bhandler = boardApp_Handler()
    app.router.add_get(board_routes[0], bhandler.rest_comp_presence)
    app.router.add_get(board_routes[1], bhandler.rest_firmware_info)
    app.router.add_get(board_routes[2], bhandler.rest_chassis_eeprom_hdl)
    app.router.add_get(board_routes[3], bhandler.rest_all_serial_and_location_hdl)
