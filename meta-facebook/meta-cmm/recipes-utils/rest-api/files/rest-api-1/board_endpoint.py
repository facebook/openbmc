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
import bottle
import rest_component_presence
import rest_firmware

boardApp = bottle.Bottle()


# Handler to fetch component presence
@boardApp.route('/api/sys/component_presence')
def rest_comp_presence():
    return rest_component_presence.get_presence()


# Handler to fetch firmware_info
@boardApp.route('/api/sys/firmware_info')
def rest_firmware_info():
    return rest_firmware.get_firmware_info()

