# Copyright 2020-present Facebook. All Rights Reserved.
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


# ELBERTTODO 442087 REST API SUPPORT
def setup_board_routes(app: Application, write_enabled: bool):
    bhandler = boardApp_Handler()
    app.router.add_get(board_routes[0], bhandler.rest_fruid_scm_hdl)
    app.router.add_get(board_routes[1], bhandler.rest_seutil_hdl)
    app.router.add_get(board_routes[2], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[3], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[4], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[5], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[6], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[7], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[8], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[9], bhandler.rest_peutil_hdl)
    app.router.add_get(board_routes[10], bhandler.rest_piminfo_hdl)
    app.router.add_get(board_routes[11], bhandler.rest_pimserial_hdl)
    app.router.add_get(board_routes[12], bhandler.rest_pimstatus_hdl)
    app.router.add_get(board_routes[13], bhandler.rest_pim_present_hdl)
    app.router.add_get(board_routes[14], bhandler.rest_smbinfo_hdl)
    app.router.add_get(board_routes[15], bhandler.rest_firmware_info_all_hdl)
    app.router.add_get(board_routes[16], bhandler.rest_sensors_scm_hdl)
    app.router.add_get(board_routes[17], bhandler.rest_sensors_smb_hdl)
    app.router.add_get(board_routes[18], bhandler.rest_sensors_psu1_hdl)
    app.router.add_get(board_routes[19], bhandler.rest_sensors_psu2_hdl)
    app.router.add_get(board_routes[20], bhandler.rest_sensors_psu3_hdl)
    app.router.add_get(board_routes[21], bhandler.rest_sensors_psu4_hdl)
    app.router.add_get(board_routes[22], bhandler.rest_sensors_pim2_hdl)
    app.router.add_get(board_routes[23], bhandler.rest_sensors_pim3_hdl)
    app.router.add_get(board_routes[24], bhandler.rest_sensors_pim4_hdl)
    app.router.add_get(board_routes[25], bhandler.rest_sensors_pim5_hdl)
    app.router.add_get(board_routes[26], bhandler.rest_sensors_pim6_hdl)
    app.router.add_get(board_routes[27], bhandler.rest_sensors_pim7_hdl)
    app.router.add_get(board_routes[28], bhandler.rest_sensors_pim8_hdl)
    app.router.add_get(board_routes[29], bhandler.rest_sensors_pim9_hdl)
    app.router.add_get(board_routes[30], bhandler.rest_sensors_fan_hdl)
