#!/usr/bin/env python
from ctypes import *


libpal_name = "libpal.so.0"


def tpm_physical_presence(fru, argv):
    option = argv[2]
    if option == "get":
        result = pal_get_tpm_physical_presence(fru)
        print("TPM physical presence=" + str(result))
    elif option == "set":
        presence = int(argv[3])
        if presence == 1:
            timeout = int(argv[4])
            # check timeout
            if timeout >= 0:
                ret = pal_set_tpm_timeout(fru, timeout)
                if ret != 0:
                    exit(-1)
                if pal_get_tpm_physical_presence(fru) == 0:
                    pal_set_tpm_physical_presence(fru, 1)
                else:
                    pal_set_tpm_physical_presence_reset(fru, 1)
            else:
                print("Enter wrong TPM timeout value")
                exit(-1)
        elif presence == 0:
            pal_set_tpm_physical_presence(fru, presence)


def pal_set_tpm_physical_presence(slot, presence):
    ret = CDLL(libpal_name).pal_set_tpm_physical_presence(slot, presence)
    if ret != 0:
        print("Error %d returned by pal_set_tpm_physical_presence" % (ret))
    return ret


def pal_get_tpm_physical_presence(slot):
    return CDLL(libpal_name).pal_get_tpm_physical_presence(slot)


def pal_set_tpm_timeout(slot, timeout):
    ret = CDLL(libpal_name).pal_set_tpm_timeout(slot, timeout)
    if ret != 0:
        print("Error %d returned by pal_set_tpm_timeout" % (ret))
    return ret


def pal_set_tpm_physical_presence_reset(slot, reset):
    ret = CDLL(libpal_name).pal_set_tpm_physical_presence_reset(slot, reset)
    if ret != 0:
        print("Error %d returned by pal_set_tpm_physical_presence_reset" % (ret))
    return ret
