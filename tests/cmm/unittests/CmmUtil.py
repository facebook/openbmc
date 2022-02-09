from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import os
try:
    currentPath = os.getcwd()
    commonPath = currentPath.replace('cmm/unittests', 'common')
    sys.path.insert(0, commonPath)
except Exception:
    pass
import BaseUtil


class CmmUtil(BaseUtil.BaseUtil):

    # Sensors
    SensorCmd = '/usr/bin/sensors'

    # Fans
    GetFanCmd = '/usr/local/bin/get_fan_speed.sh 1'
    SetFanCmd = '/usr/local/bin/set_fan_speed.sh 0 1'
    KillControlCmd = ['/usr/local/bin/wdtcli stop',
                      '/usr/bin/killall -USR1 fand']
    StartControlCmd = '/bin/sh /etc/init.d/setup-fan.sh'

    # watchdog
    watchdogDaemonKill = ['/usr/bin/killall fand']
    watchdogDaemonRestore = ['/bin/sh /etc/init.d/setup-fan.sh']

    def get_speed(self, info):
        """
        Supports getting fan speed for CMM
        """
        info = info.decode('utf-8')
        info = info.split(':')[1].split('RPM')
        speed = int(info[0])
        return speed

    def get_fan_test(self, info):
        info = info.decode('utf-8')
        info = info.split('\n')
        goodRead = ['Fan', 'RPM']
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
        return True

    # EEPROM
    ProductName = ['CMM']
    EEPROMCmd = '/usr/bin/weutil'

    # sol
    solCmd = '/usr/local/bin/sol.sh lc101'
    solCloseConnection = ['\r', 'CTRL-x']

    def solConnectionClosed(self, info):
        if 'Exit from SOL session' in info:
            return True
        else:
            return False
