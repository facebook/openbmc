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

from typing import Any, Dict, Optional

# Class Definition for Resource


class node:
    def __init__(self, info=None, actions=None):
        if info is None:
            self.info = {}
        else:
            self.info = info

        if actions is None:
            self.actions = []
        else:
            self.actions = actions

    async def getInformation(
        self, param: Optional[Dict[Any, Any]] = None
    ):  # noqa: B006
        return self.info

    def getActions(self):
        return self.actions

    async def doAction(
        self, action, param: Optional[Dict[Any, Any]] = None
    ):  # noqa: B006
        return {"result": "failure", "reason": "not supported"}
