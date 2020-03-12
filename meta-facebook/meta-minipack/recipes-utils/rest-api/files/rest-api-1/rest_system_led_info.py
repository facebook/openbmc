#!/usr/bin/env python3
import re
import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for system led info resource endpoint
def get_system_led_info() -> Dict:
    return {"Information": _get_system_led_info(), "Actions": [], "Resources": []}


def _parse_system_led_info(data: str) -> Dict:
    """ Parse led info

    Data format: <led_type>:<color>
    """
    result = {}
    for line in data.splitlines():
        if re.match(r".*:.*", line):
            split = line.split(":")
            led_type = split[0].strip()
            color = split[1].strip()
            result[led_type] = color

    return result


def _get_system_led_info() -> Dict:
    # Get the 4 system leds info: sys/smb/psu/fan
    brd_rev = _get_brd_rev()
    cmd = ["/usr/local/bin/read_sled.sh", str(brd_rev)]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_system_led_info(data)


def _get_brd_rev() -> int:
    # Get the board revision from weutil
    cmd = "weutil | grep '^Product Version' | cut -d' ' -f3"
    proc = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return int(data)
