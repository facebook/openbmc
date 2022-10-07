#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
This module provides the platform information to users.
Users can run `bios-util slotX --plat_info get` to call it.
"""

import json
import re
from subprocess import PIPE, Popen
import kv

SHOW_SYS_CONFIG = "/usr/local/bin/show_sys_config"


def is_halfdome():
    try:
        conf = kv.kv_get("sled_system_conf", kv.FPERSIST, True)
        if conf.find(b"HD") != -1:
            return True
        return False

    except Exception:
        return False


""" Get Platform Info """


def plat_info(fru):
    """Prvoide the system configuration"""

    sku = "Yosemite V3.5"
    slot_index = str(fru)
    present = ""
    sys_type = ""
    pcie_configuration = ""

    try:
        cmd = [SHOW_SYS_CONFIG, "-json"]
        proc = Popen(" ".join(cmd), shell=True, stdout=PIPE, stderr=PIPE)
        data = proc.stdout.read().decode().strip()
        json_data = json.loads(data)

        present = json_data["server_config"]["slot" + str(fru)]["Type"]
        if not re.search("not detected", present, re.I):
            present = "Present"
        else:
            print("Presense: " + present)
            return

        sys_type = json_data["Type"]
        if is_halfdome():
            sku = "HalfDome"
            pcie_configuration = "2x HalfDomes"
        elif "Class 2" in sys_type:
            pcie_configuration = "2x Crater Lakes"
        else:
            pcie_configuration = json_data["server_config"]["slot" + str(fru)][
                "Type"
            ].lower()
            if re.search(r"a|b", pcie_configuration):
                pcie_configuration = "4x Crater Lakes"
            elif re.search(r"c|d", pcie_configuration):
                pcie_configuration = "2x Crater Lakes"

        print("Presense: " + present)
        print("Non Test Board")
        print("SKU: " + sku)
        print("Slot Index: " + slot_index)
        print("PCIe Configuration: " + pcie_configuration)
    except OSError:
        print("Error: Couldn't get the system configuration")
