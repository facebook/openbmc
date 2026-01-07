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

import hashlib
import json
import re
import subprocess
from typing import Dict

from aiohttp.web_exceptions import HTTPNotFound
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


def _run_command(cmd: str) -> tuple[str, str]:
    """
    Run a shell command and return (stdout, error_message).
    """
    try:
        proc = subprocess.Popen(
            ["/usr/bin/bash", "-o", "pipefail", "-c", cmd],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        stdout, stderr = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
        stdout_str = stdout.decode().strip()
        stderr_str = stderr.decode().strip()

        if proc.returncode != 0:
            error_msg = f"Command failed with exit code {proc.returncode}: {cmd}"
            if stderr_str:
                error_msg += f" - stderr: {stderr_str}"
            return stdout_str, error_msg

        return stdout_str, ""
    except subprocess.TimeoutExpired:
        return "", f"Command timed out after {DEFAULT_TIMEOUT_SEC}s: {cmd}"
    except Exception as e:
        return "", f"Command exception: {cmd} - {repr(e)}"


def _check_condition(condition: str, entity: str) -> bool:
    """Check if a conditional firmware component should be queried."""
    cmd = condition.replace("{entity}", entity)
    proc = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    return proc.returncode == 0


def _get_firmware_versions() -> tuple[Dict[str, str], Dict[str, str]]:
    """
    Get all firmware versions and return as tuple of dicts.
    fw_key: version, fw_key: error (if applicable)
    """
    fw_data = {}
    errors = {}
    fw_manifest = _load_manifest()

    # Example UFW Manifest structure: https://fburl.com/code/0279atrc
    for fw_name, fw_config in fw_manifest.items():
        get_version_cmd = fw_config.get("get_version")
        if not get_version_cmd:
            continue

        entities = fw_config.get("entities", [None])
        condition = fw_config.get("condition")

        for entity in entities:
            if condition and entity:
                if not _check_condition(condition, entity):
                    continue

            if entity:
                cmd = get_version_cmd.replace("{entity}", entity)
                fw_key = f"{fw_name}-{entity}"
            else:
                cmd = get_version_cmd
                fw_key = fw_name

            version, error = _run_command(cmd)
            if error:
                errors[fw_key] = error

            version = _normalize_version(version)
            fw_data[fw_key] = version

    return dict(sorted(fw_data.items())), dict(sorted(errors.items()))


def get_fw_versions() -> Dict:
    """
    Returns a dict with firmware version information and a hash of all versions.
    """
    fw_versions, errors = _get_firmware_versions()
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
