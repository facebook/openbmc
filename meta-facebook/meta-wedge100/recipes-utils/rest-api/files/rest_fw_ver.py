#!/usr/bin/env python3

import functools
import subprocess
from asyncio.subprocess import PIPE, create_subprocess_exec
from typing import List

import kv


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


class kv_cache_async:
    def __init__(self, with_key):
        self.key = with_key

    def __call__(self, f):
        @functools.wraps(f)
        async def _inner(*args, **kwargs):
            try:
                res = kv.kv_get(self.key, kv.FPERSIST)
            except kv.KeyOperationFailure:
                res = await f(*args, **kwargs)
                kv.kv_set(self.key, res, kv.FPERSIST)
            return res.strip()

        return _inner


async def get_sys_cpld_ver() -> str:
    cmd = ["/usr/local/bin/cpld_rev.sh"]
    return await __run_cmd(cmd)


async def get_fan_cpld_ver() -> str:
    cmd = ["/usr/local/bin/cpld_rev.sh", "--fan"]
    return await __run_cmd(cmd)


@kv_cache_async("internal_switch_config")
async def get_internal_switch_config() -> str:
    cmd = ["/usr/local/bin/internal_switch_version.sh"]
    return await __run_cmd(cmd)
