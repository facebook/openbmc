#!/usr/bin/env python
import os.path


try:
    from lib_pal_ex import pal_print_postcode
except ImportError:
    from lib_pal import pal_print_postcode


POST_CODE_FILE = "/tmp/post_code_buffer.bin"


def postcode(fru, tmp_file):
    if os.path.isfile(tmp_file):
        with open(tmp_file, "r") as postcode_file:
            print(postcode_file.read())
    else:
        pal_print_postcode(fru)

    return True
