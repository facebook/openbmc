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
import re
try:
    currentPath = os.getcwd()
    commonPath = currentPath.replace('wedge/unittests', 'common')
    sys.path.insert(0, commonPath)
except Exception:
    pass
import BaseUtil


class WedgeUtil(BaseUtil.BaseUtil):

    # Sensors
    SensorCmd = '/usr/bin/sensors'

    # Fans
    GetFanCmd = '/usr/local/bin/get_fan_speed.sh 1'
    SetFanCmd = '/usr/local/bin/set_fan_speed.sh 0'
    KillControlCmd = ['/usr/local/bin/wdtcli stop', '/usr/bin/killall -USR1 fand']
    StartControlCmd = 'sh /etc/init.d/setup-fan.sh'

    # watchdog
    watchdogDaemonKill = ['/usr/bin/killall fand']
    watchdogDaemonRestore = ['/bin/sh /etc/init.d/setup-fan.sh']

    # Host Mac
    HostMacCmd = '/usr/local/bin/wedge_us_mac.sh'

    def get_speed(self, info):
        """
        Supports getting fan speed for wedge
        """
        info = info.decode('utf-8')
        info = info.split(':')[1].split(',')
        speed = int(info[0])
        return speed

    def get_fan_test(self, info):
        info = info.decode('utf-8')
        info = info.split('\n')
        goodRead = ['Fan', 'RPMs:']
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
            val = line.split(':')[1].split(',')
            if len(val) != 2:
                return False
        return True

    # EEPROM
    ProductName = ['Wedge']
    EEPROMCmd = '/usr/bin/weutil'

    # Power Cycle
    PowerCmdOn = '/usr/local/bin/wedge_power.sh on'
    PowerCmdOff = '/usr/local/bin/wedge_power.sh off'
    PowerCmdReset = '/usr/local/bin/wedge_power.sh reset'
    PowerCmdStatus = '/usr/local/bin/wedge_power.sh status'
    PowerHW = 'source /usr/local/bin/board-utils.sh && wedge_is_us_on && echo "on" || echo "off"'

    # sol
    solCmd = '/usr/local/bin/sol.sh'
    solCloseConnection = ['\r', 'CTRL-l', 'x']

    def solConnectionClosed(self, info):
        if 'Connection closed' in info:
            return True
        else:
            return False
