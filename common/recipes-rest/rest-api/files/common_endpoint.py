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
import rest_bmc
import rest_fcpresent
import rest_fruid
import rest_fruid_pim
import rest_fscd_sensor_data
import rest_gpios
import rest_modbus_cmd
import rest_modbus
import rest_psu_update
import rest_sensors
import rest_server
from aiohttp import web
from common_utils import (
    common_force_async,
    get_data_from_generator,
    get_endpoints,
    dumps_bytestr,
)


class commonApp_Handler:

    # Handler for root resource endpoint
    def helper_rest_api(self, request):
        result = {
            "Information": {
                "Description": "Wedge RESTful API Entry",
                "version": "v0.1",
            },
            "Actions": [],
            "Resources": get_endpoints("/api"),
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_api(self, request):
        return self.helper_rest_api(request)

    # Handler for root resource endpoint
    def helper_rest_sys(self, request):
        result = {
            "Information": {"Description": "Wedge System"},
            "Actions": [],
            "Resources": get_endpoints("/api/sys"),
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_sys(self, request):
        return self.helper_rest_sys(request)

    # Handler for sys/mb resource endpoint
    def helper_rest_mb_sys(self, request):
        result = {
            "Information": {"Description": "System Motherboard"},
            "Actions": [],
            "Resources": get_endpoints("/api/sys/mb"),
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_mb_sys(self, request):
        return self.helper_rest_mb_sys(request)

    # Handler for sys/mb/fruid resource endpoint
    def helper_rest_fruid_hdl(self, request):
        return web.json_response(rest_fruid.get_fruid(), dumps=dumps_bytestr)

    @common_force_async
    def rest_fruid_hdl(self, request):
        return self.helper_rest_fruid_hdl(request)

    # Handler for sys/mb/fruid resource endpoint
    def helper_rest_fruid_pim_hdl(self, request):
        return web.json_response(
            rest_fruid_pim.get_fruid(), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def rest_fruid_pim_hdl(self, request):
        return self.helper_rest_fruid_pim_hdl(request)

    async def rest_bmc_hdl(self, request):
        result = await rest_bmc.get_bmc()
        return web.json_response(result, dumps=dumps_bytestr)

    # Handler for sys/server resource endpoint
    def helper_rest_server_hdl(self, request):
        return web.json_response(rest_server.get_server(), dumps=dumps_bytestr)

    @common_force_async
    def rest_server_hdl(self, request):
        return self.helper_rest_server_hdl(request)

    # Handler for uServer resource endpoint
    def helper_rest_server_act_hdl(self, request):
        data = get_data_from_generator(request.json())
        return web.json_response(
            rest_server.server_action(data), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def rest_server_act_hdl(self, request):
        return self.helper_rest_server_act_hdl(request)

    # Handler for sensors resource endpoint
    def helper_rest_sensors_hdl(self, request):
        return web.json_response(
            rest_sensors.get_sensors(), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def rest_sensors_hdl(self, request):
        return self.helper_rest_sensors_hdl(request)

    # Handler for gpios resource endpoint
    def helper_rest_gpios_hdl(self, request):
        return web.json_response(
            rest_gpios.get_gpios(), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def rest_gpios_hdl(self, request):
        return self.helper_rest_gpios_hdl(request)

    # Handler for peer FC presence resource endpoint
    def helper_rest_fcpresent_hdl(self, request):
        return web.json_response(
            rest_fcpresent.get_fcpresent(), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def rest_fcpresent_hdl(self, request):
        return self.helper_rest_fcpresent_hdl(request)

    # Handler for psu_update resource endpoint
    def helper_psu_update_hdl(self, request):
        return web.json_response(
            rest_psu_update.get_jobs(), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def psu_update_hdl(self, request):
        return self.helper_psu_update_hdl(request)

    # Handler for psu_update resource action
    def helper_psu_update_hdl_post(self, request):
        data = get_data_from_generator(request.json())
        return web.json_response(
            rest_psu_update.begin_job(data), dumps=dumps_bytestr, status=200
        )

    @common_force_async
    def psu_update_hdl_post(self, request):
        return self.helper_psu_update_hdl_post(request)

    # Handler for additional fscd sensor data
    @staticmethod
    async def rest_fscd_sensor_data_post(request: web.Request) -> web.Response:
        return await rest_fscd_sensor_data.post_fscd_sensor_data(request)

    @staticmethod
    def rest_modbus_get(request: web.Request) -> web.Response:
        return web.json_response(
            {
                "Information": {"Description": "Modbus operations"},
                "Actions": [],
                "Resources": get_endpoints("/api/sys/modbus"),
            }
        )

    @staticmethod
    async def rest_modbus_cmd_post(request: web.Request) -> web.Response:
        return await rest_modbus_cmd.post_modbus_cmd(request)

    @staticmethod
    async def rest_modbus_registers_get(request: web.Request) -> web.Response:
        return web.json_response(
            await rest_modbus.get_modbus_registers(), dumps=dumps_bytestr
        )
