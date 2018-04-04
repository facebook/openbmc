#!/usr/bin/env python

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

    plat_name = pal_get_platform_name().decode()

    if 'FBTTN' in plat_name and 'identify' in command:
        fru = ''
    elif 'FBTTN' in plat_name:
        fru = 'server'
    else:
        fru = 'slot'+str(slot_id)

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
        cmd = '/usr/local/bin/power-util sled-cycle'
    elif command == 'sled-identify-on':
        cmd = '/usr/bin/fpc-util sled --identify on'
    elif command == 'sled-identify-off':
        cmd = '/usr/bin/fpc-util sled --identify off'
    else:
        return -1

    ret = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    if ret.startswith( 'Usage' ):
        return -1
    else:
        return 0

def pal_set_key_value(key, value):
    cmd = '/usr/local/bin/cfg-util %s %s' % (key,value)
    ret = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    return ret;
