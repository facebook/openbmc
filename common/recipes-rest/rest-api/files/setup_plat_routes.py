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
from typing import Dict

from aiohttp import web
from common_tree import init_common_tree
from plat_tree import init_plat_tree


def setup_plat_routes(app: web.Application, config: Dict):
    root = init_common_tree()
    root.merge(init_plat_tree())
    root.setup(app, config["writable"])
