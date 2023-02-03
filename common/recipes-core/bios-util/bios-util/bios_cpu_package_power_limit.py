#!/usr/bin/env python
import os.path
import sys
from subprocess import PIPE, Popen


def cpu_package_power_limit(fru, argv):
    # Definition for the method should be in the platform layer
    # Please check meta-facebook/meta-fbttn/../bios-util/files/bios_plat_info.py
    print("Currently the platform does not support cpu_package_power_limit")
    pass
