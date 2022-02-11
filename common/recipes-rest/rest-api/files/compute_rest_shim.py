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
from aiohttp import web
from common_utils import (
    dumps_bytestr,
    get_endpoints,
)
from node import node


class RestShim:
    # The purpose of this class is to enable using old
    # node style compute rest controllers with the fboss
    # style routing by providing a get and post action
    # for each compute node-based rest endpoint
    def __init__(self, node: node, path: str):
        self.node = node
        self.path = path

    async def get_handler(self, request):
        param = dict(request.query)
        info = await self.node.getInformation(param)
        actions = self.node.getActions()
        resources = get_endpoints(self.path)
        if "redfish" in self.path:
            result = info
        else:
            result = {
                "Information": info,
                "Actions": actions,
                "Resources": list(resources),
            }
        return web.json_response(result, dumps=dumps_bytestr)

    async def post_handler(self, request):
        result = {"result": "not-supported"}
        data = await request.json()
        param = dict(request.query)
        if "action" in data and data["action"] in self.node.actions:
            result = await self.node.doAction(data, param)
        return web.json_response(result, dumps=dumps_bytestr)


async def handlePostDefault(request):
    return web.json_response({"result": "not-supported"}, dumps=dumps_bytestr)
