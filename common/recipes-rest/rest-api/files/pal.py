#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

PAL_STATUS_UNSUPPORTED = 2

from ctypes import *
from subprocess import *
import os

try:
    lpal_hndl = CDLL("libpal.so")
except OSError:
    lpal_hndl = None

def pal_get_platform_name():
    if lpal_hndl is None:
        machine = "OpenBMC"
        with open('/etc/issue') as f:
            l = f.read().strip()
            if l.startswith("OpenBMC Release "):
                tmp = l.split(' ')
                vers = tmp[2]
                tmp2 = vers.split('-')
                machine = tmp2[0]
        return machine
    name = create_string_buffer(16)
    ret = lpal_hndl.pal_get_platform_name(name)
    if ret:
        return None
    else:
        return name.value.decode()

def pal_get_num_slots():
    if lpal_hndl is None:
        return 1
    num = c_ubyte()
    p_num = pointer(num)
    ret = lpal_hndl.pal_get_num_slots(p_num)
    if ret:
        return None
    else:
        return num.value

def pal_is_fru_prsnt(slot_id):
    if lpal_hndl is None:
        return None
    status = c_ubyte()
    p_status = pointer(status)
    ret = lpal_hndl.pal_is_fru_prsnt(slot_id, p_status)
    if ret:
        return None
    else:
        return status.value

def pal_get_server_power(slot_id):
    # TODO Use wedge_power.sh?
    if lpal_hndl is None:
        return None
    status = c_ubyte()
    p_status = pointer(status)
    ret = lpal_hndl.pal_get_server_power(slot_id, p_status)
    if ret:
        return None
    else:
        return status.value

# return value
#  1 - bic okay
#  0 - bic error
#  2 - not present
def pal_get_bic_status(slot_id):
    if lpal_hndl is None:
        return 0
    plat_name = pal_get_platform_name()
    if 'FBTTN' in plat_name:
        fru = 'server'
    elif 'FBY2' in plat_name or 'Yosemite' in plat_name:
        fru = 'slot'+str(slot_id)
    elif 'minipack' in plat_name:
        fru = 'scm'
    else:
        return PAL_STATUS_UNSUPPORTED

    cmd = ['/usr/bin/bic-util', fru, '--get_dev_id']

    try:
        ret = check_output(cmd).decode()
        if "Usage:" in ret or "fail " in ret:
            return 0
        else:
            return 1
    except (OSError, IOError):
        return PAL_STATUS_UNSUPPORTED # No bic on this platform
    except(CalledProcessError):
        return 0  # bic-util returns error

def pal_server_action(slot_id, command, fru_name = None):
    # TODO use wedge_power.sh?
    if lpal_hndl is None:
        return -1
    if command == 'power-off' or command == 'power-on' or command == 'power-reset' or command == 'power-cycle' or command == 'graceful-shutdown':
        if lpal_hndl.pal_is_slot_server(slot_id) == 0:
            return -2

    plat_name = pal_get_platform_name()

    if 'FBTTN' in plat_name and 'identify' in command:
        fru = ''
    elif 'FBTTN' in plat_name and fru_name is None:
        fru = 'server'
    elif fru_name is None:
        fru = 'slot'+str(slot_id)
    else:
        fru = fru_name

    if command == 'power-off':
        cmd = '/usr/local/bin/power-util '+fru+' off'
    elif command == 'power-on':
        cmd = '/usr/local/bin/power-util '+fru+' on'
    elif command == 'power-reset':
        cmd = '/usr/local/bin/power-util '+fru+' reset'
    elif command == 'power-cycle':
        cmd = '/usr/local/bin/power-util '+fru+' cycle'
    elif command == 'graceful-shutdown':
        cmd = '/usr/local/bin/power-util '+fru+' graceful-shutdown'
    elif command == '12V-off':
        cmd = '/usr/local/bin/power-util '+fru+' 12V-off'
    elif command == '12V-on':
        cmd = '/usr/local/bin/power-util '+fru+' 12V-on'
    elif command == '12V-cycle':
        cmd = '/usr/local/bin/power-util '+fru+' 12V-cycle'
    elif command == 'identify-on':
        cmd = '/usr/bin/fpc-util '+fru+' --identify on'
    elif command == 'identify-off':
        cmd = '/usr/bin/fpc-util '+fru+' --identify off'
    else:
        return -1
    ret = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    if ret.find("Usage:") != -1 or ret.find("fail ") != -1:
        return -1
    else:
        return 0

def pal_sled_action(command):
    if command == 'sled-cycle':
        cmd = ['/usr/local/bin/power-util', 'sled-cycle']
    elif command == 'sled-identify-on':
        cmd = ['/usr/bin/fpc-util', 'sled', '--identify', 'on']
    elif command == 'sled-identify-off':
        cmd = ['/usr/bin/fpc-util', 'sled', '--identify', 'off']
    else:
        return -1
    try:
        ret = check_output(cmd).decode()
        if ret.startswith( 'Usage' ):
            return -1
        else:
            return 0
    except (OSError, IOError, CalledProcessError):
        return -1

def pal_set_key_value(key, value):
    cmd = ['/usr/local/bin/cfg-util', key, value]
    if (os.path.exists(cmd[0])):
        output = check_output(cmd).decode()
        if "Usage:" in output:
            raise ValueError("failure")
    else:
        pkey = c_char_p(key.encode())
        pvalue = c_char_p(value.encode())
        ret = lpal_hndl.pal_set_key_value(pkey, pvalue)
        if (ret != 0):
            raise ValueError("failure")
