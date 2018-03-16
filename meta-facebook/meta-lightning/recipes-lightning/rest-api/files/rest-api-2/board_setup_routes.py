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
from boardroutes import board_routes


def setup_board_routes(app):
    bhandler = boardApp_Handler()
    app.router.add_get(board_routes[0], bhandler.rest_api)
    app.router.add_get(board_routes[1], bhandler.rest_peb)
    app.router.add_get(board_routes[2], bhandler.rest_pdpb)
    app.router.add_get(board_routes[3], bhandler.rest_fcb)
    app.router.add_get(board_routes[4], bhandler.rest_peb_fruid_hdl)
    app.router.add_get(board_routes[5], bhandler.rest_peb_sensor_hdl)
    app.router.add_get(board_routes[6], bhandler.rest_peb_bmc_hdl)
    app.router.add_get(board_routes[7], bhandler.rest_peb_health_hdl)
    app.router.add_get(board_routes[8], bhandler.rest_peb_logs)
    app.router.add_get(board_routes[9], bhandler.rest_pdpb_sensors_hdl)
    app.router.add_get(board_routes[10], bhandler.rest_pdpb_flash_hdl)
    app.router.add_get(board_routes[11], bhandler.rest_pdpb_fruid_hdl)
    app.router.add_get(board_routes[12], bhandler.rest_pdpb_logs)
    app.router.add_get(board_routes[13], bhandler.rest_fcb_fans_hdl)
    app.router.add_get(board_routes[14], bhandler.rest_fcb_fruid_hdl)
    app.router.add_get(board_routes[15], bhandler.rest_fcb_sensors_hdl)
    app.router.add_get(board_routes[16], bhandler.rest_fcb_logs)
