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
import json
import rest_fruid
import rest_server
import rest_sensors
import rest_bmc
import rest_gpios
import rest_modbus
import rest_slotid
import rest_psu_update
import rest_fcpresent
import rest_mTerm
from aiohttp import web
from rest_utils import get_endpoints

class commonApp_Handler:

    # Handler for root resource endpoint
    async def rest_api(self, request):
        result = {
            "Information": {
                "Description": "Wedge RESTful API Entry",
            },
            "Actions": [],
            "Resources": get_endpoints('/api')
        }
        return web.json_response(result)

    # Handler for root resource endpoint
    async def rest_sys(self,request):
        result = {
            "Information": {
                "Description": "Wedge System",
            },
            "Actions": [],
            "Resources": get_endpoints('/api/sys')
        }
        return web.json_response(result)

    # Handler for sys/mb resource endpoint
    async def rest_mb_sys(self, request):
        result = {
            "Information": {
                "Description": "System Motherboard",
            },
            "Actions": [],
            "Resources": get_endpoints('/api/sys/mb')
        }
        return web.json_response(result)

    # Handler for sys/mb/fruid resource endpoint
    async def rest_fruid_hdl(self,request):
        return web.json_response(rest_fruid.get_fruid())

    # Handler for sys/bmc resource endpoint
    async def rest_bmc_hdl(self,request):
        return web.json_response(rest_bmc.get_bmc())

    # Handler for sys/server resource endpoint
    async def rest_server_hdl(self,request):
        return web.json_response(rest_server.get_server())

    # Handler for uServer resource endpoint
    async def rest_server_act_hdl(self,request):
        data = await request.json()
        return web.json_response(rest_server.server_action(data))

    # Handler for sensors resource endpoint
    async def rest_sensors_hdl(self,request):
        return web.json_response(rest_sensors.get_sensors())

    # Handler for gpios resource endpoint
    async def rest_gpios_hdl(self,request):
        return web.json_response(rest_gpios.get_gpios())

    # Handler for peer FC presence resource endpoint
    async def rest_fcpresent_hdl(self,request):
        return web.json_response(rest_fcpresent.get_fcpresent())

    # Handler for Modbus_registers resource endpoint
    async def modbus_registers_hdl(self,request):
        return web.json_response(rest_modbus.get_modbus_registers())

    # Handler for psu_update resource endpoint
    async def psu_update_hdl(self,request):
        return web.json_response(rest_psu_update.get_jobs())

    # Handler for psu_update resource action
    async def psu_update_hdl_post(self,request):
        data = await request.json()
        return web.json_response(rest_psu_update.begin_job(data))

    # Handler for get slotid from endpoint
    async def rest_slotid_hdl(self,request):
        return web.json_response(rest_slotid.get_slotid())

    # Handler for mTerm status
    async def rest_mTerm_status(self,request):
        return web.json_response(rest_mTerm.get_mTerm_status())
