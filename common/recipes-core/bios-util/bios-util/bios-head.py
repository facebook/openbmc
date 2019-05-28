#!/usr/bin/env python
import os.path
import re
import sys
from ctypes import *
from subprocess import PIPE, Popen

from bios_board import *
from lib_pal import *


def bios_main():
    fruname = sys.argv[1]
    command = sys.argv[2]

    if fruname == "all":
        frulist_c = create_string_buffer(128)
        ret = CDLL(libpal_name).pal_get_fru_list(frulist_c)
        if ret:
            print("Getting fru list failed!")
            return
        frulist_s = frulist_c.value.decode()
        frulist = re.split(r",\s", frulist_s)
        for fruname in frulist:
            fru = pal_get_fru_id(fruname)
            if fru >= 0:
                if pal_is_slot_server(fru) == True:
                    print("%s:" % (fruname))
                    bios_main_fru(fru, command)
    else:
        fru = pal_get_fru_id(fruname)
        if fru < 0:
            print("%s is not a known FRU on this platform" % (fruname))
            return
        if pal_is_slot_server(fru) == False:
            print("%s is not a server" % (fruname))
            return
        bios_main_fru(fru, command)


if __name__ == "__main__":
    bios_usage_check = check_bios_util()
    if bios_usage_check.valid is True:
        bios_main()
