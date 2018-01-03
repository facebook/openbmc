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
import asyncio
from aiohttp import web
from rest_utils import dumps_bytestr, get_endpoints
from concurrent.futures import ThreadPoolExecutor

def common_force_async(func):
    async def func_wrapper(self, *args, **kwargs):
        # Convert the possibly blocking helper function into async
        loop = asyncio.get_event_loop()
        result = await loop.run_in_executor(self.common_executor, \
                                            func, self, *args, **kwargs)
        return result
    return func_wrapper

class commonApp_Handler:

    # common handler will use its own executor (thread based),
    # we initentionally separated this from the executor of
    # board-specific REST handler, so that any problem in
    # common REST handlers will not interfere with board-specific
    # REST handler, and vice versa
    def __init__(self):
        # Max number of concurrent thread is set to 5,
        # in order to ensure enough concurrency while
        # not overloading CPU too much
        self.common_executor = ThreadPoolExecutor(5)

    # When we call request.json() in asynchronous function, a generator
    # will be returned. Upon calling next(), the generator will either :
    #
    # 1) return the next data as usual,
    #   - OR -
    # 2) throw StopIteration, with its first argument as the data
    #    (this is for indicating that no more data is available)
    #
    # Not sure why aiohttp's request generator is implemented this way, but
    # the following function will handle both of the cases mentioned above.
    def get_data_from_generator(self, data_generator):
        data = None
        try:
            data = next(data_generator)
        except StopIteration as e:
            data = e.args[0]
        return data

    # Handler for root resource endpoint
    def helper_rest_api(self, request):
        result = {
            "Information": {
                "Description": "Wedge RESTful API Entry",
            },
            "Actions": [],
            "Resources": get_endpoints('/api')
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_api(self, request):
        return self.helper_rest_api(request)

    # Handler for root resource endpoint
    def helper_rest_sys(self,request):
        result = {
            "Information": {
                "Description": "Wedge System",
            },
            "Actions": [],
            "Resources": get_endpoints('/api/sys')
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_sys(self,request):
        return self.helper_rest_sys(request)

    # Handler for sys/mb resource endpoint
    def helper_rest_mb_sys(self, request):
        result = {
            "Information": {
                "Description": "System Motherboard",
            },
            "Actions": [],
            "Resources": get_endpoints('/api/sys/mb')
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_mb_sys(self, request):
        return self.helper_rest_mb_sys(request)

    # Handler for sys/mb/fruid resource endpoint
    def helper_rest_fruid_hdl(self,request):
        return web.json_response(rest_fruid.get_fruid(), dumps=dumps_bytestr)

    @common_force_async
    def rest_fruid_hdl(self,request):
        return self.helper_rest_fruid_hdl(request)

    # Handler for sys/bmc resource endpoint
    def helper_rest_bmc_hdl(self,request):
        result = rest_bmc.get_bmc()
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_bmc_hdl(self,request):
        return self.helper_rest_bmc_hdl(request)

    # Handler for sys/server resource endpoint
    def helper_rest_server_hdl(self,request):
        return web.json_response(rest_server.get_server(), dumps=dumps_bytestr)

    @common_force_async
    def rest_server_hdl(self,request):
        return self.helper_rest_server_hdl(request)

    # Handler for uServer resource endpoint
    def helper_rest_server_act_hdl(self,request):
        data = self.get_data_from_generator(request.json())
        return web.json_response(rest_server.server_action(data), dumps=dumps_bytestr)

    @common_force_async
    def rest_server_act_hdl(self,request):
        return self.helper_rest_server_act_hdl(request)

    # Handler for sensors resource endpoint
    def helper_rest_sensors_hdl(self,request):
        return web.json_response(rest_sensors.get_sensors(), dumps=dumps_bytestr)

    @common_force_async
    def rest_sensors_hdl(self,request):
        return self.helper_rest_sensors_hdl(request)

    # Handler for gpios resource endpoint
    def helper_rest_gpios_hdl(self,request):
        return web.json_response(rest_gpios.get_gpios(), dumps=dumps_bytestr)

    @common_force_async
    def rest_gpios_hdl(self,request):
        return self.helper_rest_gpios_hdl(request)

    # Handler for peer FC presence resource endpoint
    def helper_rest_fcpresent_hdl(self,request):
        return web.json_response(rest_fcpresent.get_fcpresent(), dumps=dumps_bytestr)

    @common_force_async
    def rest_fcpresent_hdl(self,request):
        return self.helper_rest_fcpresent_hdl(request)

    # Handler for Modbus_registers resource endpoint
    def helper_modbus_registers_hdl(self,request):
        return web.json_response(rest_modbus.get_modbus_registers(), dumps=dumps_bytestr)

    @common_force_async
    def modbus_registers_hdl(self,request):
        return self.helper_modbus_registers_hdl(request)

    # Handler for psu_update resource endpoint
    def helper_psu_update_hdl(self,request):
        return web.json_response(rest_psu_update.get_jobs(), dumps=dumps_bytestr)

    @common_force_async
    def psu_update_hdl(self,request):
        return self.helper_psu_update_hdl(request)

    # Handler for psu_update resource action
    def helper_psu_update_hdl_post(self,request):
        data = self.get_data_from_generator(request.json())
        return web.json_response(rest_psu_update.begin_job(data), dumps=dumps_bytestr)

    @common_force_async
    def psu_update_hdl_post(self,request):
        return self.helper_psu_update_hdl_post(request)

    # Handler for get slotid from endpoint
    def helper_rest_slotid_hdl(self,request):
        return web.json_response(rest_slotid.get_slotid(), dumps=dumps_bytestr)

    @common_force_async
    def rest_slotid_hdl(self,request):
        return self.helper_rest_slotid_hdl(request)

    # Handler for mTerm status
    def helper_rest_mTerm_status(self,request):
        return web.json_response(rest_mTerm.get_mTerm_status(), dumps=dumps_bytestr)

    @common_force_async
    def rest_mTerm_status(self,request):
        return self.helper_rest_mTerm_status(request)
