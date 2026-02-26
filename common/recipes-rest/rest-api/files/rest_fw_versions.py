#!/usr/bin/env python3
#
# Copyright 2024-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

import asyncio
import hashlib
import json
import re
import shlex
from typing import Dict, Optional, Tuple

from aiohttp.web_exceptions import HTTPNotFound
from common_utils import async_exec
from rest_utils import DEFAULT_TIMEOUT_SEC

_REGEX_VERSION_PATTERN = r"^[v]?([0-9]*)\.([0-9]*)$"
_MANIFEST_FILE = "/etc/ufw_manifest.json"


def _load_manifest() -> Dict:
    """Load the firmware manifest from JSON file."""
    try:
        with open(_MANIFEST_FILE, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        raise HTTPNotFound(reason=f"Manifest file not found: {_MANIFEST_FILE}")


def _normalize_version(version: str) -> str:
    """Normalize version string to X.Y format."""
    if num_ver := re.search(_REGEX_VERSION_PATTERN, version):
        return f"{num_ver.group(1)}.{num_ver.group(2)}"
    return version


async def _run_command(cmd: str) -> Tuple[str, str]:
    """
    Run a shell command and return (stdout, error_message).
    """
    try:
        returncode, stdout, stderr = await async_exec(cmd, shell=True)
        stdout = stdout.strip()
        stderr = stderr.strip()

        if returncode != 0:
            error_msg = f"Command failed with exit code {returncode}: {cmd}"
            if stderr:
                error_msg += f" - stderr: {stderr}"
            return stdout, error_msg

        return stdout, ""
    except Exception as e:
        return "", f"Command exception: {cmd} - {repr(e)}"


async def _check_condition(condition: str, entity: str) -> bool:
    """Check if a conditional firmware component should be queried."""
    cmd = condition.replace("{entity}", entity)
    returncode, _, _ = await async_exec(cmd, shell=True)
    return returncode == 0


async def _fetch_group_versions(
    base_cmd: str, filter_cmds: Dict[str, Optional[str]]
) -> Dict[str, Tuple[str, str]]:
    """
    Run base_cmd once, then apply each filter_cmd to extract individual versions.
    Returns {fw_key: (version, error)}.
    """
    try:
        stdout, error = await _run_command(base_cmd)
        if error:
            return {fw_key: ("", error) for fw_key in filter_cmds}

        results = {}
        for fw_key, filter_cmd in filter_cmds.items():
            if filter_cmd is None:
                results[fw_key] = (stdout, "")
            else:
                cmd = f"printf '%s\\n' {shlex.quote(stdout)} | {filter_cmd}"
                results[fw_key] = await _run_command(cmd)

        return results
    except Exception as e:
        return {fw_key: ("", repr(e)) for fw_key in filter_cmds}


async def _build_groups(
    fw_manifest: Dict,
) -> Dict[str, Dict[str, Optional[str]]]:
    """
    Build {base_cmd: {fw_key: filter_cmd_or_none}} from the manifest.
    Commands sharing the same base (before the first "|") are grouped so the
    expensive hardware query runs only once per unique base command.
    """
    groups: Dict[str, Dict[str, Optional[str]]] = {}
    for fw_name, fw_config in fw_manifest.items():
        get_version_cmd = fw_config.get("get_version")
        if not get_version_cmd:
            continue

        entities = fw_config.get("entities")
        condition = fw_config.get("condition")

        if entities:
            for entity in entities:
                if condition and not await _check_condition(condition, entity):
                    continue

                cmd = get_version_cmd.replace("{entity}", entity)
                fw_key = f"{fw_name}-{entity}"

                parts = cmd.split("|", 1)
                base_cmd = parts[0].strip()
                filter_cmd = parts[1].strip() if len(parts) == 2 else None
                groups.setdefault(base_cmd, {})[fw_key] = filter_cmd
        else:
            parts = get_version_cmd.split("|", 1)
            base_cmd = parts[0].strip()
            filter_cmd = parts[1].strip() if len(parts) == 2 else None
            groups.setdefault(base_cmd, {})[fw_name] = filter_cmd

    return groups


async def _get_firmware_versions() -> Tuple[Dict[str, str], Dict[str, str]]:
    """
    Get all firmware versions, deduplicating shared base commands.
    """
    groups = await _build_groups(_load_manifest())

    group_results = await asyncio.gather(
        *[
            asyncio.wait_for(
                _fetch_group_versions(base_cmd, filter_cmds),
                timeout=DEFAULT_TIMEOUT_SEC,
            )
            for base_cmd, filter_cmds in groups.items()
        ],
        return_exceptions=True,
    )

    fw_data: Dict[str, str] = {}
    errors: Dict[str, str] = {}
    for filter_cmds, result in zip(groups.values(), group_results):
        if isinstance(result, Exception):
            for fw_key in filter_cmds:
                fw_data[fw_key] = ""
                errors[fw_key] = repr(result)
            continue
        for fw_key, (version, error) in result.items():
            fw_data[fw_key] = _normalize_version(version)
            if error:
                errors[fw_key] = error

    return dict(sorted(fw_data.items())), dict(sorted(errors.items()))


async def get_fw_versions() -> Dict:
    """
    Returns a dict with firmware version information and a hash of all versions.
    """
    fw_versions, errors = await _get_firmware_versions()
    fw_concat = ",".join(["{}:{}".format(key, val) for key, val in fw_versions.items()])

    fw_hash = hashlib.blake2s(fw_concat.encode(), digest_size=6).hexdigest()

    result = {
        "Information": {
            "firmware_versions": fw_versions,
            "firmware_string": fw_concat,
            "firmware_hash": fw_hash,
            "errors": errors,
        },
        "Actions": [],
        "Resources": [],
    }

    return result
