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

import json
import os
import socket
import ssl
from ctypes import *
from time import sleep

from node_api import get_node_api
from node_bios import *
from node_bmc import get_node_bmc
from node_config import get_node_config
from node_fans import get_node_fans
from node_fruid import get_node_fruid
from node_logs import get_node_logs
from node_mezz import get_node_mezz
from node_sensors import get_node_sensors
from node_server import get_node_device, get_node_server
from node_spb import get_node_spb
from rest_pal_legacy import *
from tree import tree
from redfish_service_root import *
from redfish_account_service import get_account_service
from redfish_chassis import *
from redfish_managers import *

from aiohttp.web import Application

# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):
    # /redfish end point
    r_redfish = tree("redfish", data=get_redfish())
    # /redfish/v1 end point
    r_root = tree("v1", data=get_service_root())
    r_redfish.addChild(r_root)
    # /redfish/v1/AccountService end point
    r_account = tree("AccountService", data=get_account_service())

    # /redfish/v1/Chasis end point
    r_chassis = tree("Chassis", data=get_chassis())
    r_root.addChildren([r_account, r_chassis])
    # /redfish/v1/Chasis/1 end point
    r_chassis_mem = tree("1", data=get_chassis_members("smb"))
    r_chassis.addChild(r_chassis_mem)
    # /redfish/v1/Chasis/1/Power end point
    #r_power = tree("Power", data=get_chassis_power("psu1", "PSU1_IN_POWER", "0x05"))
    
    # tuple(fru_name, snr_name, snr_num)
    psu1 = ("psu1", "PSU1_IN_POWER", "0x05")
    psu2 = ("psu2", "PSU2_IN_POWER", "0x0E")
    r_power = tree("Power", data=get_chassis_power([psu1,psu2]))
    # /redfish/v1/Chasis/1/Thermal end point
    r_thermal = tree("Thermal", data=get_chassis_thermal("smb"))
    r_chassis_mem.addChildren([r_power, r_thermal])

    # /redfish/v1/Managers end point
    r_managers = tree("Managers", data=get_managers())
    r_root.addChild(r_managers)
    # /redfish/v1/Managers/1 end point
    r_managers_mem = tree("1", data=get_managers_members())
    r_managers.addChild(r_managers_mem)
    # /redfish/v1/Managers/1/EthernetInterfaces end point
    r_ethernet = tree("EthernetInterfaces", data=get_manager_ethernet())
    # /redfish/v1/Managers/1/NetworkProtocol end point
    r_network = tree("NetworkProtocol", data=get_manager_network())
    # /redfish/v1/Managers/1/LogService end point
    agc032a = ['all', 'sys', 'smb', 'psu1', 'psu2', 
               'fan1', 'fan2', 'fan3', 'fan4', 'fan5', 'fan6']
    r_log = tree("LogServices", data=get_manager_logservice(agc032a))
    r_managers_mem.addChildren([r_ethernet, r_network, r_log])

    # /redfish/v1/Managers/1/EthernetInterfaces/1 end point
    r_ethernet_mem = tree("1", data=get_ethernet_members())
    r_ethernet.addChild(r_ethernet_mem)

    # /redfish/v1/Managers/1/LogService/fru end point
    log_members = []
    for item in agc032a:
        fru = str(item)
        r_log_mem = tree(fru, data=get_logservice_members(fru))
        # /redfish/v1/Managers/1/LogService/fru/Entries end point
        r_log_entry = tree('Entries', data=get_log_entries(fru))
        # /redfish/v1/Managers/1/LogService/fru/Actions end point
        r_log_action = tree("Actions", data=get_log_actions(fru))
        r_clear_log = tree("LogService.ClearLog", data=get_node_logs(id))
        r_log_action.addChild(r_clear_log)
        r_log_mem.addChildren([r_log_entry, r_log_action])
        log_members.append(r_log_mem)
    r_log.addChildren(log_members)


    r_redfish.setup(app, write_enabled)
