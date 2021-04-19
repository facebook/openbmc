#!/usr/bin/env python
import os.path
import sys

from pal import pal_get_postcode


POST_CODE_FILE = "/tmp/post_code_buffer.bin"


def postcode(fru, tmp_file):
    if os.path.isfile(tmp_file):
        with open(tmp_file, "r") as postcode_file:
            print(postcode_file.read())
    else:
        try:
            postcodes = pal_get_postcode(fru)
        except ValueError:
            return False
        for i in range(0, len(postcodes)):
            sys.stdout.write("%02X " % (postcodes[i]))
            sys.stdout.flush()
            if i % 16 == 15:
                sys.stdout.write("\n")
                sys.stdout.flush()
        if len(postcodes) % 16 != 0:
            sys.stdout.write("\n")

    return True
