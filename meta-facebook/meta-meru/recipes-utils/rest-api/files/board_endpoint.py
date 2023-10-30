#!/usr/bin/env python3
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

import rest_bmc_board_rev
from aiohttp import web
from common_utils import dumps_bytestr


class boardApp_Handler:
    # Handler for sys/bmc_board_rev endpoint
    async def rest_bmc_board_rev_hdl(self, request):
        return web.json_response(
            rest_bmc_board_rev.get_bmc_board_rev(), dumps=dumps_bytestr
        )
