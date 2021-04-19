#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
import json
import syslog

from aiohttp import web

# cache for endpoint_children
ENDPOINT_CHILDREN = {}
# aiohttp allows users to pass a "dumps" function, which will convert
# different data types to JSON. This new dumps function will simply call
# the original dumps function, along with the new type handler that can
# process byte strings.
def dumps_bytestr(obj):
    # aiohttp's json_response function uses py3 JSON encoder, which
    # doesn't know how to handle a byte string. So we extend this function
    # to handle the case. This is a standard way to add a new type,
    # as stated in JSON encoder source code.
    def default_bytestr(o):
        # If the object is a byte string, it will be converted
        # to a regular string. Otherwise we move on (pass) to
        # the usual error generation routine
        try:
            o = o.decode("utf-8")
            return o
        except AttributeError:
            pass
        raise TypeError(repr(o) + " is not JSON serializable")

    # Just call default dumps function, but pass the new default function
    # that is capable of process byte strings.
    return json.dumps(obj, default=default_bytestr)


async def handlePostDefault(request):
    return web.json_response({"result": "not-supported"}, dumps=dumps_bytestr)


# Class Definition for Tree
class tree:
    def __init__(self, name, data=None):
        self.name = name
        self.data = data
        self.children = []
        self.path = "/" + self.name
        self.app = None

    def addChild(self, child):
        self.children.append(child)

    def addChildren(self, children):
        for child in children:
            self.children.append(child)

    def getChildren(self):
        return self.children

    def getChildByName(self, name):
        for child in self.children:
            if child.name == name:
                return child
        return None

    async def handleGet(self, request):
        param = dict(request.query)
        info = self.data.getInformation(param)
        actions = self.data.getActions()
        resources = set()
        if ENDPOINT_CHILDREN.get(request.path):
            resources = ENDPOINT_CHILDREN[request.path]
        else:
            for resource in self.app.router.resources():
                if (
                    resource.url().startswith(request.path)
                    and resource.url() != request.path
                ):
                    r = resource.url().replace(request.path, "").split("/")[1]
                    resources.add(r)
            ENDPOINT_CHILDREN[request.path] = resources
        if "redfish" in self.path:
            result = info
        else:
            result = {"Information": info, "Actions": actions, "Resources": list(resources)}
        return web.json_response(result, dumps=dumps_bytestr)

    async def handlePost(self, request):
        result = {"result": "not-supported"}
        data = await request.json()
        param = dict(request.query)
        if "action" in data and data["action"] in self.data.actions:
            result = self.data.doAction(data, param)
        return web.json_response(result, dumps=dumps_bytestr)

    def setup(self, app, support_post):
        app.router.add_get(self.path, self.handleGet)
        if len(self.data.getActions()) > 0 and support_post:
            app.router.add_post(self.path, self.handlePost)
        else:
            app.router.add_post(self.path, handlePostDefault)
        for child in self.children:
            child.path = self.path + "/" + child.name
            child.setup(app, support_post)
        self.app = app
