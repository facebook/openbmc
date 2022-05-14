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

from aiohttp.web import Application
from compute_rest_shim import RestShim
from node_fruid import get_node_fruid
from rest_pal_legacy import pal_is_fru_prsnt, pal_get_num_slots

# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Add /api/fruid end point
    fruid_shim = RestShim(get_node_fruid("all"), "/api/fruid")
    app.router.add_get(fruid_shim.path, fruid_shim.get_handler)
