#!/usr/bin/env python
import sys

from pal import pal_get_4byte_postcode_lcc

POST_CODE_FILE = "/tmp/post_code_buffer.bin"


def postcode(fru, tmp_file):
    try:
        postcodes = pal_get_4byte_postcode_lcc(fru)
    except ValueError:
        return False
    except Exception as e:
        print(e)
        return False

    for i in range(0, len(postcodes)):
        sys.stdout.write("[%08X] " % (postcodes[i]))
        sys.stdout.flush()
        if i % 4 == 3:
            sys.stdout.write("\n")
            sys.stdout.flush()
    if len(postcodes) % 4 != 0:
        sys.stdout.write("\n")

    return True