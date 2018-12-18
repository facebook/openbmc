#!/usr/bin/env python3

import subprocess

from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for seutil resource endpoint
def get_seutil() -> Dict:
    return {"Information": get_seutil_data(), "Actions": [], "Resources": []}


def _parse_seutil_data(data) -> Dict:
    result = {}
    # need to remove the first info line from seutil
    adata = data.split('\n', 1)
    for sdata in adata[1].split('\n'):
        tdata = sdata.split(':', 1)
        if (len(tdata) < 2):
            continue
        result[tdata[0].strip()] = tdata[1].strip()
    return result


def get_seutil_data() -> Dict:
    cmd = ["/usr/local/bin/seutil"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors='ignore')
    return _parse_seutil_data(data)
