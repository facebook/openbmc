#!/usr/bin/env python
import sys
from pal import pal_get_page_postcode

POST_CODE_FILE = "/tmp/post_code_buffer.bin"

SBMR_POSTCODE_SIZE = 9
MAX_POST_CODE_PAGES = 17

def postcode(fru, _tmp_file):
    try:
        postcodes = pal_get_page_postcode(fru, range(0, MAX_POST_CODE_PAGES))
    except ValueError as e:
        return False
    except Exception as e:
        print(e)
        return False

    for i in range(0, len(postcodes), SBMR_POSTCODE_SIZE):
        chunk = postcodes[i:i+SBMR_POSTCODE_SIZE]
        hex_string = ''.join(format(code, '02X') for code in chunk)
        sys.stdout.write(f'[{hex_string}]\n')
        sys.stdout.flush()

    if len(postcodes) % SBMR_POSTCODE_SIZE != 0:
        sys.stdout.write("\n")

    return True