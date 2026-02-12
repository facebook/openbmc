#!/usr/bin/env python
import sys
from pal import pal_get_postcode

POST_CODE_FILE = "/tmp/post_code_buffer.bin"

def postcode(fru, tmp_file):

    try:
        postcodes = pal_get_postcode(fru)
    except ValueError:
        return False
    except Exception as e:
        print(f"Error fetching postcodes: {e}")
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
