#!/usr/bin/env python3
import re
import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


def get_all_fw_ver() -> Dict:
    return {"Information": _get_all_fw_ver(), "Actions": [], "Resources": []}


def _parse_all_cpld_ver(data) -> Dict:
    result = {}
    for line in data.splitlines():
        if re.match(r".*:.*", line):
            split = line.split(":")
            key = split[0].strip()
            value = split[1].strip()
            result[key] = value
    return result


def _parse_all_fpga_ver(data) -> Dict:
    # Parsing PIM version is a bit tricky
    result = {}
    PIM_number = "0"
    for line in data.splitlines():
        # Check if this line just shows PIM number (not version),
        # if so, update PIM number to be used in the key.
        # Here, we expects a line like: "PIM 1: "
        match = re.match(r"^PIM [1-8]", line)
        if match:
            PIM_number = match.group(0).replace("PIM ", "").strip()
        # Check if this line shows the FPGA version, if so,
        # add type and version to the dictionary
        # Here, we expects a line like "16Q DOMFPGA: 4.6"
        if re.match(r".* DOMFPGA:", line):
            split = line.split(" DOMFPGA:")
            pim_type = split[0].strip()
            pim_ver = split[1].strip()
            result["PIM" + PIM_number] = pim_ver
            result["PIM" + PIM_number + "_TYPE"] = pim_type
    return result


def _get_all_fw_ver() -> Dict:
    #
    # First, get all cpld version
    #
    cmd = ["/usr/local/bin/cpld_ver.sh"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    result_cpld = _parse_all_cpld_ver(data)
    #
    # Secondly, get all fpga version, a bit tricky
    #
    cmd = ["/usr/local/bin/fpga_ver.sh"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    result_fpga = _parse_all_fpga_ver(data)
    #
    # Finally, we combine all the results into one
    #
    result = {**result_cpld, **result_fpga}
    return result
