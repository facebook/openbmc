#!/usr/bin/env python3

import re
import subprocess

from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for sys/firmware_info/pim resource endpoint
def get_firmware_info() -> Dict:
    return {"Information": get_cpld_info_data(), "Actions": [], "Resources": []}


def _parse_cpld_info_data(data) -> Dict:
    result = {}
    for sdata in data.splitlines():
        if re.match(r"^SCMCPLD", sdata):
            result["1"] = sdata.replace("SCMCPLD: ", "")
    return result


def get_cpld_info_data() -> Dict:
    cmd = ["/usr/local/bin/cpld_ver.sh"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors='ignore')
    return _parse_cpld_info_data(data)
