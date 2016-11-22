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
import rest_fruid_scm
import rest_seutil
import rest_sol
import rest_usb2i2c_reset
import rest_firmware

boardApp = bottle.Bottle()


# Handler for sys/mb/fruid_scm resource endpoint
@boardApp.route('/api/sys/mb/fruid_scm')
def rest_fruid_scm_hdl():
  return rest_fruid_scm.get_fruid_scm()


# Handler for sys/mb/seutil resource endpoint
@boardApp.route('/api/sys/mb/seutil')
def rest_seutil_hdl():
  return rest_seutil.get_seutil()


# Handler for SOL resource endpoint
@boardApp.route('/api/sys/sol', method='POST')
def rest_sol_act_hdl():
    data = json.load(request.body)
    return rest_sol.sol_action(data)


# Handler to reset usb-to-i2c
@boardApp.route('/api/sys/usb2i2c_reset')
def rest_usb2i2c_reset_hdl():
    return rest_usb2i2c_reset.set_usb2i2c()


# Handler to get firmware info
@boardApp.route('/api/sys/firmware_info')
def rest_firmware_info():
    return rest_firmware.get_firmware_info()
