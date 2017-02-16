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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import subprocess
import syslog


def get_all_serials_and_location():
    api = 'http://[fe80::c1:1%eth0.4088]:8080/api/sys/mb/chassis_all_serial_and_location'
    proc = subprocess.Popen(
        ['/usr/bin/wget', '--timeout', '30', '-q', '-O', '-', api],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)

    try:
        data, err = proc.communicate()
        if proc.returncode:
            raise Exception('Command failed, returncode: {}'.format(
                proc.returncode))
        return data
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, 'Error getting SN, Location from CMM: {}'
                      .format(e))
        return None


if __name__ == '__main__':
    print(get_all_serials_and_location())
