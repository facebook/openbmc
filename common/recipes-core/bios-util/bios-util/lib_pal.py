#!/usr/bin/env python
import sys
from ctypes import CDLL, byref, c_ubyte, c_uint


libpal_name = "libpal.so.0"


def pal_print_postcode(fru):
    postcodes = (c_ubyte * 256)()
    plen = c_uint(0)
    ret = CDLL(libpal_name).pal_get_80port_record(
        fru, byref(postcodes), 256, byref(plen)
    )
    if ret != 0:
        print("Error %d returned by get_80port" % (ret))
        return
    for i in range(0, plen.value):
        sys.stdout.write("%02X " % (postcodes[i]))
        sys.stdout.flush()
        if i % 16 == 15:
            sys.stdout.write("\n")
            sys.stdout.flush()
    if plen.value % 16 != 0:
        sys.stdout.write("\n")
    sys.stdout.flush()


def pal_get_fru_id(fruname):
    fruid = c_ubyte(0)
    ret = CDLL(libpal_name).pal_get_fru_id(fruname.encode("ascii"), byref(fruid))
    if ret != 0:
        return ret
    return fruid.value


def pal_is_slot_server(fru):
    ret = CDLL(libpal_name).pal_is_slot_server(c_ubyte(fru))
    if ret != 1:
        return False
    return True
