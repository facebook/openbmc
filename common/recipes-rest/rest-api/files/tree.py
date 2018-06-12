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
from aiohttp import web
import json
import syslog


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
           o = o.decode('utf-8')
           return o
        except AttributeError:
           pass
        raise TypeError(repr(o) + " is not JSON serializable")
    # Just call default dumps function, but pass the new default function
    # that is capable of process byte strings.
    return json.dumps(obj, default=default_bytestr)

# Class Definition for Tree

class tree:
    def __init__(self, name, data = None):
        self.name = name
        self.path = '/' + name
        self.data = data
        self.children = []

    def addChild(self, child):
        self.children.append(child)
        child.path = self.path + '/' + child.name

    def addChildren(self, children):
        for child in children:
            self.children.append(child)
            child.path = self.path + '/' + child.name

    def getChildren(self):
        return self.children

    def getChildByName(self, name):
        if self.name == name:
            return self

        for child in self.children:
            if child.name == name:
                return child
        return None

    async def handleGet(self, request):
        param=dict(request.query)
        info = self.data.getInformation(param)
        actions = self.data.getActions()
        resources = []
        ca = self.getChildren()
        for t in ca:
            resources.append(t.name)
        result = {
                'Information': info,
                'Actions': actions,
                'Resources': resources
        }
        return web.json_response(result, dumps=dumps_bytestr)

    async def handlePost(self, request):
        result = {'result': 'booo'}
        data = await request.json()
        if 'action' in data:
            result = self.data.doAction(data, False)
        return web.json_response(result, dumps=dumps_bytestr)

    def setup(self, app, support_post):
        app.router.add_get(self.path, self.handleGet)
        if len(self.data.getActions()) > 0 and support_post:
            app.router.add_post(self.path, self.handlePost)
        for child in self.children:
            child.setup(app, support_post)
