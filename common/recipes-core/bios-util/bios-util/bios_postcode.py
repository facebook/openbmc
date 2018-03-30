#!/usr/bin/env python
import sys
import re
import os.path
from subprocess import Popen, PIPE
from ctypes import *
from lib_pal import *
from bios_ipmi_util import *


POST_CODE_FILE = "/tmp/post_code_buffer.bin"

def postcode(fru, tmp_file):
    if os.path.isfile(tmp_file):
        postcode_file = open(tmp_file, 'r')
        print(postcode_file.read())
    else:
        pal_print_postcode(fru)
    
    return True

