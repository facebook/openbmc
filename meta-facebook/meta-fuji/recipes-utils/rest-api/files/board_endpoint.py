#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

import rest_feutil
import rest_firmware_info
import rest_peutil
import rest_pim_present
import rest_piminfo
import rest_pimserial
import rest_presence
import rest_sensors
import rest_seutil
import rest_smbinfo
import rest_system_led_info
from aiohttp import web
from common_utils import dumps_bytestr


class boardApp_Handler:
    # Handler for sys/seutil resource endpoint
    async def rest_seutil_hdl(self, request):
        return web.json_response(rest_seutil.get_seutil(), dumps=dumps_bytestr)

    # Handler for sys/firmware_info resource endpoint
    async def rest_firmware_info_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/cpld resource endpoint
    async def rest_firmware_info_cpld_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_cpld(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/fpga resource endpoint
    async def rest_firmware_info_fpga_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_fpga(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/scm resource endpoint
    async def rest_firmware_info_scm_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_scm(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/all resource endpoint
    async def rest_firmware_info_all_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_all(), dumps=dumps_bytestr
        )

    # Handler for sys/presence resource endpoint
    async def rest_presence_hdl(self, request):
        return web.json_response(rest_presence.get_presence_info(), dumps=dumps_bytestr)

    # Handler for sys/presence/scm resource endpoint
    async def rest_presence_scm_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_scm(), dumps=dumps_bytestr
        )

    # Handler for sys/presence/fan resource endpoint
    async def rest_presence_fan_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_fan(), dumps=dumps_bytestr
        )

    # Handler for sys/presence/psu resource endpoint
    async def rest_presence_psu_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_psu(), dumps=dumps_bytestr
        )

    # Handler for sys/presence/pim resource endpoint
    async def rest_presence_pim_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_pim(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil resource endpoint
    async def rest_feutil_hdl(self, request):
        return web.json_response(rest_feutil.get_feutil(), dumps=dumps_bytestr)

    # Handler for sys/feutil/all resource endpoint
    async def rest_feutil_all_hdl(self, request):
        return web.json_response(rest_feutil.get_feutil_all_data(), dumps=dumps_bytestr)

    # Handler for sys/feutil/fcm-t resource endpoint
    async def rest_feutil_fcm_t_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fcm_t_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fcm-b resource endpoint
    async def rest_feutil_fcm_b_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fcm_b_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan1 resource endpoint
    async def rest_feutil_fan1_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan1_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan2 resource endpoint
    async def rest_feutil_fan2_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan2_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan3 resource endpoint
    async def rest_feutil_fan3_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan3_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan4 resource endpoint
    async def rest_feutil_fan4_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan4_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan5 resource endpoint
    async def rest_feutil_fan5_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan5_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan6 resource endpoint
    async def rest_feutil_fan6_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan6_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan7 resource endpoint
    async def rest_feutil_fan7_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan7_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan8 resource endpoint
    async def rest_feutil_fan8_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan8_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil resource endpoint
    async def rest_peutil_hdl(self, request):
        return web.json_response(rest_peutil.get_peutil(), dumps=dumps_bytestr)

    # Handler for sys/peutil/pim1 resource endpoint
    async def rest_peutil_pim1_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim1_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim2 resource endpoint
    async def rest_peutil_pim2_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim2_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim3 resource endpoint
    async def rest_peutil_pim3_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim3_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim4 resource endpoint
    async def rest_peutil_pim4_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim4_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim5 resource endpoint
    async def rest_peutil_pim5_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim5_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim6 resource endpoint
    async def rest_peutil_pim6_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim6_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim7 resource endpoint
    async def rest_peutil_pim7_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim7_data(), dumps=dumps_bytestr
        )

    # Handler for sys/peutil/pim8 resource endpoint
    async def rest_peutil_pim8_hdl(self, request):
        return web.json_response(
            rest_peutil.get_peutil_pim8_data(), dumps=dumps_bytestr
        )

    # Handler for sys/sensors/scm resource endpoint
    async def rest_sensors_scm_hdl(self, request):
        return web.json_response(rest_sensors.get_scm_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/smb resource endpoint
    async def rest_sensors_smb_hdl(self, request):
        return web.json_response(rest_sensors.get_smb_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu1 resource endpoint
    async def rest_sensors_psu1_hdl(self, request):
        return web.json_response(rest_sensors.get_psu1_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu2 resource endpoint
    async def rest_sensors_psu2_hdl(self, request):
        return web.json_response(rest_sensors.get_psu2_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu3 resource endpoint
    async def rest_sensors_psu3_hdl(self, request):
        return web.json_response(rest_sensors.get_psu3_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu4 resource endpoint
    async def rest_sensors_psu4_hdl(self, request):
        return web.json_response(rest_sensors.get_psu4_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim1 resource endpoint
    async def rest_sensors_pim1_hdl(self, request):
        return web.json_response(rest_sensors.get_pim1_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim2 resource endpoint
    async def rest_sensors_pim2_hdl(self, request):
        return web.json_response(rest_sensors.get_pim2_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim3 resource endpoint
    async def rest_sensors_pim3_hdl(self, request):
        return web.json_response(rest_sensors.get_pim3_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim4 resource endpoint
    async def rest_sensors_pim4_hdl(self, request):
        return web.json_response(rest_sensors.get_pim4_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim5 resource endpoint
    async def rest_sensors_pim5_hdl(self, request):
        return web.json_response(rest_sensors.get_pim5_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim6 resource endpoint
    async def rest_sensors_pim6_hdl(self, request):
        return web.json_response(rest_sensors.get_pim6_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim7 resource endpoint
    async def rest_sensors_pim7_hdl(self, request):
        return web.json_response(rest_sensors.get_pim7_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim8 resource endpoint
    async def rest_sensors_pim8_hdl(self, request):
        return web.json_response(rest_sensors.get_pim8_sensors(), dumps=dumps_bytestr)

    # Handler for sys/pim_serial resource endpoint
    async def rest_pimserial_hdl(self, request):
        return web.json_response(rest_pimserial.get_pimserial(), dumps=dumps_bytestr)

    # Handler for sys/pim_present resource endpoint
    # This is redundant, but infra service need this interface.
    async def rest_pim_present_hdl(self, request):
        return web.json_response(
            rest_pim_present.get_pim_present(), dumps=dumps_bytestr
        )

    # Handler for sys/system_led_info endpoint
    async def rest_system_led_info_hdl(self, request):
        return web.json_response(
            rest_system_led_info.get_system_led_info(), dumps=dumps_bytestr
        )

    # Handler for sys/piminfo resource endpoint
    async def rest_piminfo_hdl(self, request):
        return web.json_response(rest_piminfo.get_piminfo())

    # Handler for sys/smbinfo resource endpoint
    async def rest_smbinfo_hdl(self, request):
        return web.json_response(rest_smbinfo.get_smbinfo(), dumps=dumps_bytestr)
