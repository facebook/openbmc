#!/usr/bin/env python3

import subprocess
from asyncio.subprocess import PIPE, create_subprocess_exec
from typing import Dict


async def get_all_fw_ver() -> Dict:
    cmd = "/usr/local/bin/cpld_rev.sh"
    proc = await create_subprocess_exec(cmd, stdout=PIPE)
    data, _ = await proc.communicate()
    await proc.wait()
    return {"SYS_CPLD": data.decode(errors="ignore").strip()}
