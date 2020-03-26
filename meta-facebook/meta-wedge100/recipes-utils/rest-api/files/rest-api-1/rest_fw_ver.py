#!/usr/bin/env python3

import subprocess
from asyncio.subprocess import PIPE, create_subprocess_exec
from typing import List


async def __run_cmd(cmd: List[str]) -> str:
    proc = await create_subprocess_exec(*cmd, stdout=PIPE, stderr=PIPE)
    data, proc_stderr = await proc.communicate()
    await proc.wait()
    result = data.decode(errors="ignore").rstrip()
    if proc.returncode:
        raise subprocess.CalledProcessError(
            cmd=cmd, returncode=proc.returncode, stderr=proc_stderr.decode()
        )
    else:
        return result


async def get_sys_cpld_ver() -> str:
    cmd = ["/usr/local/bin/cpld_rev.sh"]
    return await __run_cmd(cmd)


async def get_fan_cpld_ver() -> str:
    cmd = ["/usr/local/bin/cpld_rev.sh", "--fan"]
    return await __run_cmd(cmd)
