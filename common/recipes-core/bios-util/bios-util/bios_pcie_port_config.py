#!/usr/bin/env python
import os.path
import sys
from subprocess import PIPE, Popen

from bios_ipmi_util import *


def pcie_port_config(fru, argv):
    # Definition for the method should be in the platform layer
    # Please check meta-facebook/meta-fbttn/../bios-util/files/bios_pcie_port_config.py
    print("Currently the platform does not support pcie_port_config")
    pass
