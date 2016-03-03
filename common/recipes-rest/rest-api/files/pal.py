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

from ctypes import *
from subprocess import *

lpal_hndl = CDLL("libpal.so")

def pal_get_platform_name():
    name = create_string_buffer(16)
    ret = lpal_hndl.pal_get_platform_name(name)
    if ret:
        return None
    else:
        return name.value

def pal_get_num_slots():
    num = c_ubyte()
    p_num = pointer(num)
    ret = lpal_hndl.pal_get_num_slots(p_num)
    if ret:
        return None
    else:
        return num.value

def pal_is_fru_prsnt(slot_id):
    status = c_ubyte()
    p_status = pointer(status)
    ret = lpal_hndl.pal_is_fru_prsnt(slot_id, p_status)
    if ret:
        return None
    else:
        return status.value

def pal_get_server_power(slot_id):
    status = c_ubyte()
    p_status = pointer(status)
    ret = lpal_hndl.pal_get_server_power(slot_id, p_status)
    if ret:
        return None
    else:
        return status.value

def pal_server_action(slot_id, command):
    sid = str(slot_id)
    if command == 'power-off':
        cmd = '/usr/local/bin/power-util slot'+sid+' off'
    elif command == 'power-on':
        cmd = '/usr/local/bin/power-util slot'+sid+' on'
    elif command == 'power-cycle':
        cmd = '/usr/local/bin/power-util slot'+sid+' cycle'
    elif command == 'graceful-shutdown':
        cmd = '/usr/local/bin/power-util slot'+sid+' graceful-shutdown'
    elif command == '12V-off':
        cmd = '/usr/local/bin/power-util slot'+sid+' 12V-off'
    elif command == '12V-on':
        cmd = '/usr/local/bin/power-util slot'+sid+' 12V-on'
    elif command == '12V-cycle':
        cmd = '/usr/local/bin/power-util slot'+sid+' 12V-cycle'
    elif command == 'identify-on':
        cmd = '/usr/bin/fpc-util slot'+sid+' --identify on'
    elif command == 'identify-off':
        cmd = '/usr/bin/fpc-util slot'+sid+' --identify off'
    else:
        return -1
    ret = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    if ret.startswith( 'Usage' ):
        return -1
    else:
        return 0

def pal_sled_action(command):
    if command == 'sled-cycle':
        cmd = '/usr/local/bin/power-util sled-cycle'
    elif command == 'sled-identify-on':
        cmd = '/usr/bin/fpc-util sled --identify on'
    elif command == 'sled-identify-off':
        cmd = '/usr/bin/fpc-util sled --identify off'
    else:
        return -1

    ret = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
    if ret.startswith( 'Usage' ):
        return -1
    else:
        return 0

def pal_set_key_value(key, value):
    pkey = create_string_buffer(key)
    pvalue = create_string_buffer(value)

    ret = lpal_hndl.pal_set_key_value(pkey, pvalue)
    if ret:
        return -1;
    else:
        return 0;
