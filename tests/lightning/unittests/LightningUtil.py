from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import os
import re
try:
    currentPath = os.getcwd()
    commonPath = currentPath.replace('lightning/unittests', 'common')
    sys.path.insert(0, commonPath)
except Exception:
    pass
import BaseUtil


class LightningUtil(BaseUtil.BaseUtil):

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

    # LightningConnectionTest path
    ConnectionTestPath = "tests/lightning/unittests/lightningConnectionTest.py"

    def get_speed(self, info):
        """
        Supports getting fan pwm for Lightning
        """
        info = info.decode('utf-8')
        info = info.split(':')[1].split('RPM')
        pwm_str = re.sub("[^0-9]", "", info[1])
        print(pwm_str)
        return int(pwm_str)

    def get_fan_test(self, info):
        info = info.decode().split('\n')
        goodRead = ['Fan', 'RPM', '%', 'Speed']
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
        return True

    # EEPROM
    ProductName = ['Lightning']
    EEPROMCmd = '/usr/local/bin/fruid-util pdpb'

    # FPC
    FPCCmdOn = "/usr/bin/fpc-util --identify on"
    FPCCmdOff = "/usr/bin/fpc-util --identify off"
