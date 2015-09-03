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

# Class Definition for Tree

class tree:
    def __init__(self, name, data = None):
        self.name = name
        self.data = data
        self.children = []

    def addChild(self, child):
        self.children.append(child)

    def addChildren(self, children):
        for child in children:
            self.children.append(child)

    def getChildren(self):
        return self.children

    def getChildByName(self, name):
        if self.name == name:
            return self

        for child in self.children:
            if child.name == name:
                return child
        return None
