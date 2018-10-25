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
import abc


class BaseUtil(object):
    # Sensors
    @abc.abstractproperty
    def SensorCmd(self):
        pass

    # Fans
    @abc.abstractproperty
    def GetFanCmd(self):
        pass

    @abc.abstractproperty
    def SetFanCmd(self):
        pass

    @abc.abstractproperty
    def KillControlCmd(self):
        pass

    @abc.abstractproperty
    def StartControlCmd(self):
        pass

    @abc.abstractmethod
    def get_speed(self, info):
        """
        Supports getting fan speed or pwm (if present)
        """
        return

    @abc.abstractmethod
    def get_fan_test(self, info):
        """
        Supports checking format of get fan dump of info
        """
        return

    # EEPROM
    @abc.abstractproperty
    def ProductName(self):
        pass

    @abc.abstractproperty
    def EEPROMCmd(self):
        pass

    # Power Cycle
    @abc.abstractproperty
    def PowerCmdOn(self):
        pass

    @abc.abstractproperty
    def PowerCmdOff(self):
        pass

    @abc.abstractproperty
    def PowerCmdReset(self):
        pass

    @abc.abstractproperty
    def PowerCmdStatus(self):
        pass

    @abc.abstractproperty
    def PowerHW(self):
        pass

    # sol
    @abc.abstractproperty
    def solCmd(self):
        pass

    @abc.abstractproperty
    def solCloseConnection(self):
        pass

    @abc.abstractmethod
    def solConnectionClosed(self, info):
        """
        Supports checking that the sol connection closed properly
        """

    # watchdog
    @abc.abstractproperty
    def watchdogDaemonKill(self):
        pass

    @abc.abstractproperty
    def watchdogDaemonRestore(self):
        pass
