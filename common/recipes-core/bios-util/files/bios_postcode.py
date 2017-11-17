#!/usr/bin/env python
import sys
import re
import os.path
from subprocess import Popen, PIPE
from ctypes import *
from lib_pal import *
from bios_ipmi_util import *


POST_CODE_FILE = "/tmp/post_code_buffer.bin"

def postcode(fru):
    if os.path.isfile(POST_CODE_FILE):
        postcode_file = open(POST_CODE_FILE, 'r')
        print(postcode_file.read())
    else:
        pal_print_postcode(fru)

