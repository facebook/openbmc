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
from aiohttp import web
from plat_tree import init_plat_tree
from rest_utils import dumps_bytestr
from boardroutes import board_routes


def url_router(path, root):
    if path.startswith('/'):
        path = path[len('/'):]
    token = path.split('/')
    # Find the Node
    r = root
    for t in token:
        r = r.getChildByName(t)
        if r is None:
            return r
    c = r.data

    info = c.getInformation()
    actions = c.getActions()

    resources = []
    ca = r.getChildren()
    for t in ca:
        resources.append(t.name)
    result = {'Information': info,
              'Actions': actions,
              'Resources': resources}
    return result


class boardApp_Handler:
    def __init__(self):
        self.root = init_plat_tree()

    # Handler for /api resource endpoint
    async def rest_api(self, request):
        path = board_routes[0]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/peb resource endpoint
    async def rest_peb(self, request):
        path = board_routes[1]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/pdpb resource endpoint
    async def rest_pdpb(self, request):
        path = board_routes[2]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/fcb resource endpoint
    async def rest_fcb(self, request):
        path = board_routes[3]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/peb/fruid resource endpoint
    async def rest_peb_fruid_hdl(self, request):
        path = board_routes[4]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/peb/sensors resource endpoint
    async def rest_peb_sensor_hdl(self, request):
        path = board_routes[5]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/peb/bmc resource endpoint
    async def rest_peb_bmc_hdl(self, request):
        path = board_routes[6]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/peb/health resource endpoint
    async def rest_peb_health_hdl(self, request):
        path = board_routes[7]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/peb/logs resource endpoint
    async def rest_peb_logs(self, request):
        path = board_routes[8]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/pdpb/sensors resource endpoint
    async def rest_pdpb_sensors_hdl(self, request):
        path = board_routes[9]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/pdpb/flash resource endpoint
    async def rest_pdpb_flash_hdl(self, request):
        path = board_routes[10]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/pdpb/fruid resource endpoint
    async def rest_pdpb_fruid_hdl(self, request):
        path = board_routes[11]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/pdpb/logs resource endpoint
    async def rest_pdpb_logs(self, request):
        path = board_routes[12]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/fcb/fans resource endpoint
    async def rest_fcb_fans_hdl(self, request):
        path = board_routes[13]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/fcb/fruid resource endpoint
    async def rest_fcb_fruid_hdl(self, request):
        path = board_routes[14]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/fcb/sensors resource endpoint
    async def rest_fcb_sensors_hdl(self, request):
        path = board_routes[15]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)

    # Handler for /api/fcb/logs resource endpoint
    async def rest_fcb_logs(self, request):
        path = board_routes[16]
        return web.json_response(url_router(path, self.root), dumps=dumps_bytestr)
