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

from node import node
from rest_pal_legacy import pal_get_platform_name

class gtNode(node):
    def __init__(self, info=None, actions=None):
        if info == None:
            self.info = {}
        else:
            self.info = info

        if actions == None:
            self.actions = []
        else:
            self.actions = actions

def get_node_scm():
    name = pal_get_platform_name()
    info = {"Description": name + " SCM Board"}

    return gtNode(info)

def get_node_swb():
    name = pal_get_platform_name()
    info = {"Description": name + " Switch Board"}

    return gtNode(info)

def get_node_vpdb():
    name = pal_get_platform_name()
    info = {"Description": name + " VPDB Board"}

    return gtNode(info)

def get_node_hpdb():
    name = pal_get_platform_name()
    info = {"Description": name + " HPDB Board"}

    return gtNode(info)

def get_node_fan_bp1():
    name = pal_get_platform_name()
    info = {"Description": name + " FAN BP1 Board"}

    return gtNode(info)

def get_node_fan_bp2():
    name = pal_get_platform_name()
    info = {"Description": name + " FAN BP2 Board"}

    return gtNode(info)

def get_node_hgx():
    name = pal_get_platform_name()
    info = {"Description": name + " HGX Board"}

    return gtNode(info)



