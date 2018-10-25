#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import os
try:
    currentPath = os.getcwd()
    commonPath = currentPath.replace('galaxy100/unittests', 'common')
    sys.path.insert(0, commonPath)
except Exception:
    pass
import BaseUtil


class Galaxy100Util(BaseUtil.BaseUtil):

    # Sensors
    SensorCmd = '/usr/bin/sensors'

    # EEPROM
    ProductName = ['LC', 'FAB']
    EEPROMCmd = '/usr/bin/weutil'

    # Power Cycle
    PowerCmdOn = '/usr/local/bin/wedge_power.sh on'
    PowerCmdOff = '/usr/local/bin/wedge_power.sh off'
    PowerCmdReset = '/usr/local/bin/wedge_power.sh reset'
    PowerCmdStatus = '/usr/local/bin/wedge_power.sh status'
    PowerHW = 'source /usr/local/bin/board-utils.sh && wedge_is_us_on && echo "on" || echo "off"'

    # watchdog
    watchdogDaemonKill = ['/usr/bin/killall watchdogd.sh']
    watchdogDaemonRestore = ['/bin/sh /etc/init.d/setup-watchdogd.sh']

    # sol
    solCmd = '/usr/local/bin/sol.sh'
    solCloseConnection = ['\r', 'CTRL-l', 'x']

    def solConnectionClosed(self, info):
        if 'Connection closed' in info:
            return True
        else:
            return False

    # Host Mac
    HostMacCmd = '/usr/local/bin/wedge_us_mac.sh'
