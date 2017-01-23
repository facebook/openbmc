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

from ctypes import *
import json
import ssl
import socket
import os
from node_api import get_node_api
from node_scc import get_node_scc
from node_nic import get_node_nic
from node_dpb import get_node_dpb
from node_iom import get_node_iom
from node_server import get_node_server
from node_bmc import get_node_bmc
from node_fruid import get_node_fruid
from node_sensors import get_node_sensors
from node_logs import get_node_logs
from node_config import get_node_config
from node_health import get_node_health
from node_identify import get_node_identify
from node_fans import get_node_fans
from tree import tree
from pal import *

def populate_server_node(num):
    prsnt = pal_is_fru_prsnt(num)
    if prsnt == None or prsnt == 0:
        return None

    r_server = tree("server" + repr(num), data = get_node_server(num))
    r_fruid = tree("fruid", data = get_node_fruid("slot" + repr(num)))
    r_sensors = tree("sensors", data = get_node_sensors("slot" + repr(num)))
    r_logs = tree("logs", data = get_node_logs("slot" + repr(num)))
    r_config = tree("config", data = get_node_config("slot" + repr(num)))
    r_server.addChildren([r_fruid, r_sensors, r_logs, r_config])

    return r_server


# Initialize Platform specific Resource Tree
def init_plat_tree():

    # Create /api end point as root node
    r_api = tree("api", data = get_node_api())


    # Add /api/nic to represent Mezzanine Card
    r_nic = tree("nic", data = get_node_nic())
    r_api.addChild(r_nic)
    # Add /api/iom to represent IO Module
    r_iom = tree("iom", data = get_node_iom())
    r_api.addChild(r_iom)
    # Add /api/dpb to represent Drive Plan Board
    r_dpb = tree("dpb", data = get_node_dpb())
    r_api.addChild(r_dpb)
    # Add /api/scc to represent Storage Controller Card
    r_scc = tree("scc", data = get_node_scc())
    r_api.addChild(r_scc)

    # Add servers /api/slot[1-max]
    num = pal_get_num_slots()
    for i in range(1, num+1):
        r_server = populate_server_node(i)
        if r_server:
            r_api.addChild(r_server)
   
    #Add /api/nic/fruid end point
    r_temp = tree("fruid", data = get_node_fruid("nic"))
    r_nic.addChild(r_temp)
    #Add /api/nic/sensors end point
    r_temp = tree("sensors", data = get_node_sensors("nic"))
    r_nic.addChild(r_temp)
    #Add /api/nic/logs end point
    r_temp = tree("logs", data = get_node_logs("nic"))
    r_nic.addChild(r_temp)

    #Add /api/iom/fruid end point
    r_temp = tree("fruid", data = get_node_fruid("iom"))
    r_iom.addChild(r_temp)
    #Add /api/iom/sensors end point
    r_temp = tree("sensors", data = get_node_sensors("iom"))
    r_iom.addChild(r_temp)
    #Add /api/iom/logs end point
    r_temp = tree("logs", data = get_node_logs("iom"))
    r_iom.addChild(r_temp)
    #Add /api/iom/bmc end point
    r_temp = tree("bmc", data = get_node_bmc())
    r_iom.addChild(r_temp)
    #Add /api/iom/health end point
    r_temp = tree("health", data = get_node_health())
    r_iom.addChild(r_temp)
    #Add /api/iom/identify end point
    r_temp = tree("identify", data = get_node_identify("iom"))
    r_iom.addChild(r_temp)

    #Add /api/dpb/fruid end point
    r_temp = tree("fruid", data = get_node_fruid("dpb"))
    r_dpb.addChild(r_temp)
    #Add /api/dpb/sensors end point
    r_temp = tree("sensors", data = get_node_sensors("dpb"))
    r_dpb.addChild(r_temp)
    #Add /api/dpb/logs end point
    r_temp = tree("logs", data = get_node_logs("dpb"))
    r_dpb.addChild(r_temp)
    #Add /api/dpb/fans end point
    r_temp = tree("fans", data = get_node_fans())
    r_dpb.addChild(r_temp)

    #Add /api/scc/fruid end point
    r_temp = tree("fruid", data = get_node_fruid("scc"))
    r_scc.addChild(r_temp)
    #Add /api/scc/sensors end point
    r_temp = tree("sensors", data = get_node_sensors("scc"))
    r_scc.addChild(r_temp)
    #Add /api/scc/logs end point
    r_temp = tree("logs", data = get_node_logs("scc"))
    r_scc.addChild(r_temp)

    return r_api
