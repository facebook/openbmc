# Copyright 2018-present Facebook. All Rights Reserved.
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

from aiohttp.web import Application


def setup_board_routes(app: Application, write_enabed: bool):
    bhandler = boardApp_Handler()
    app.router.add_get(board_routes[0], bhandler.rest_pim_present_hdl)
    app.router.add_get(board_routes[1], bhandler.rest_piminfo_hdl)
    app.router.add_get(board_routes[2], bhandler.rest_fruid_scm_hdl)
    app.router.add_get(board_routes[3], bhandler.rest_pimserial_hdl)
    app.router.add_get(board_routes[4], bhandler.rest_pimstatus_hdl)
    app.router.add_get(board_routes[5], bhandler.rest_seutil_hdl)
    app.router.add_get(board_routes[6], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[7], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[8], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[9], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[10], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[11], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[12], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[13], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[14], bhandler.rest_scdinfo_hdl)
    app.router.add_get(board_routes[15], bhandler.rest_firmware_info_all_hdl)
    app.router.add_get(board_routes[16], bhandler.rest_firmware_info_hdl)
    app.router.add_get(board_routes[17], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[18], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[19], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[20], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[21], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[22], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[23], bhandler.rest_pim_endpoint_info_hdl)
    app.router.add_get(board_routes[24], bhandler.rest_pim_endpoint_info_hdl)
