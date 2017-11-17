#!/usr/bin/env python
import sys
import os.path
from subprocess import Popen, PIPE
from bios_ipmi_util import *


def plat_info(fru):
    # Definition for the method should be in the platform layer
    # Please check meta-facebook/meta-fbttn/../bios-util/files/bios_plat_info.py
    print("Currently the platform does not support plat_info")
    pass

