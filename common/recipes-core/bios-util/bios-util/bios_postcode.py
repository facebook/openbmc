#!/usr/bin/env python
import os.path
import re
import sys
from ctypes import *
from subprocess import PIPE, Popen

from bios_ipmi_util import *
from lib_pal import *


POST_CODE_FILE = "/tmp/post_code_buffer.bin"


def postcode(fru, tmp_file):
    if os.path.isfile(tmp_file):
        postcode_file = open(tmp_file, "r")
        print(postcode_file.read())
    else:
        pal_print_postcode(fru)

    return True
