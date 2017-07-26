#!/usr/bin/env python3
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
import subprocess

def read_gpio_sysfs(gpio):    
    try:
        with open('/sys/class/gpio/gpio%d/value' % gpio, 'r') as f:
            val_string = f.read()
            if val_string == '1\n':
                return 1
            if val_string == '0\n':
                return 0
    except:
        return None

def get_wedge_slot():
    p = subprocess.Popen('source /usr/local/bin/openbmc-utils.sh;'
                         'wedge_slot_id $(wedge_board_type)',
                         shell=True, stdout=subprocess.PIPE)
    out, err = p.communicate()
    out = out.decode()
    try:
        slot = int(out.strip('\n'))
    except:
        slot = 0
    return slot
