#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
This module provides the platform information to users.
Users can run `bios-util slotX --plat_info get` to call it.
"""

from subprocess import PIPE, Popen
import json
import re

SHOW_SYS_CONFIG = "/usr/local/bin/show_sys_config"

""" Get Platform Info """
def plat_info(fru):
    """ Prvoide the system configuration """

    sku = "Yosemite V3"
    slot_index = str(fru)
    present = ""
    sys_type = ""
    pcie_configuration = ""

    try:
        cmd = [SHOW_SYS_CONFIG, "-json"]
        proc = Popen(' '.join(cmd), shell=True, stdout=PIPE, stderr=PIPE)
        data = proc.stdout.read().decode().strip()
        json_data = json.loads(data)

        present = json_data["server_config"]["slot"+str(fru)]["Type"]
        if not re.search('not present', present, re.I):
            present = "Present"

        sys_type = json_data["Type"]
        if "Class 2" in sys_type:
            pcie_configuration = "2x Delta Lakes"
        else:
            pcie_configuration = json_data["server_config"]["slot"+str(fru)]["Type"].lower()
            if re.search(r'a|b', pcie_configuration):
                pcie_configuration = "4x Delta Lakes"
            elif re.search(r'c|d', pcie_configuration):
                pcie_configuration = "2x Delta Lakes"

        print ("Presense: " + present)
        print ("Non Test Board")
        print ("SKU: " + sku)
        print ("Slot Index: " + slot_index)
        print ("PCIe Configuration: " + pcie_configuration)
    except OSError:
        print ("Error: Couldn't get the system configuration")
