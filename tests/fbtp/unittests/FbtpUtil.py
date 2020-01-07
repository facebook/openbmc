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
    commonPath = currentPath.replace('fbtp/unittests', 'common')
    sys.path.insert(0, commonPath)
except OSError:
    pass
import BaseUtil

class FbtpUtil(BaseUtil.BaseUtil):

    # Sensors
    SensorCmd = '/usr/local/bin/sensor-util all'

    # Fans
    GetFanCmd = '/usr/local/bin/fan-util --get 0'
    SetFanCmd = '/usr/local/bin/fan-util --set 0'
    KillControlCmd = ['/usr/bin/sv stop fscd']
    StartControlCmd = '/usr/bin/sv start fscd'

    # Watchdog
    watchdogDaemonKill = ['/usr/bin/sv stop fscd', '/usr/bin/sv stop healthd']
    watchdogDaemonRestore = ['/usr/bin/sv start healthd', '/usr/bin/sv start fscd']

    def get_speed(self, info):
        """
        Supports getting fan pwm for Tioga Pass
        """
        info = info.decode('utf-8')
        info = info.split(':')[1].split('RPM')
        pwm_str = re.sub("[^0-9]", "", info[1])
        return int(pwm_str)


    def get_fan_test(self, info):
        info = info.decode('utf-8')
        info = info.split('\n')
        goodRead = ['Fan', 'RPM', '%', 'Speed']
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
        return True

    # EEPROM
    ProductName = ['Tioga Pass']
    EEPROMCmd = '/usr/local/bin/fruid-util mb'

    # Power Cycle
    PowerCmdOn = '/usr/local/bin/power-util mb on'
    PowerCmdOff = '/usr/local/bin/power-util mb off'
    PowerCmdReset = '/usr/local/bin/power-util mb reset'
    PowerCmdStatus = '/usr/local/bin/power-util mb status'
    PowerHW = '/usr/local/bin/power-util mb status && echo "on" || echo "off"'

    # sol
    solCmd = '/usr/local/bin/sol-util mb'
    solCloseConnection = ['\r', 'CTRL-l', 'x']

    def solConnectionClosed(self, info):
        if 'Connection closed' in info:
            return True
        else:
            return False
