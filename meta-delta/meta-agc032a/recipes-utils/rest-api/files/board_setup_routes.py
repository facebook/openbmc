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
from node_bios import (
    get_node_bios,
    get_node_bios_boot_order_trunk,
    get_node_bios_postcode_trunk,
    get_node_bios_plat_info_trunk,
    get_node_bios_boot_mode,
    get_node_bios_clear_cmos,
    get_node_bios_force_boot_setup,
    get_node_bios_boot_order,
)
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_server
from node_spb import get_node_spb
from rest_pal_legacy import pal_is_fru_prsnt, pal_get_num_slots

# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):
    pass
