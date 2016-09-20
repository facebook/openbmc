from ctypes import *
import subprocess

lpal_hndl = CDLL("libpal.so")


def pal_fan_dead_handle(fan):
    ret = lpal_hndl.pal_fan_dead_handle(fan)
    if ret:
        return None
    else:
        return ret

def pal_fan_recovered_handle(fan):
    ret = lpal_hndl.pal_fan_recovered_handle(fan)
    if ret:
        return None
    else:
        return ret
