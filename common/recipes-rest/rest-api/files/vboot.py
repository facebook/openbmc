#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
import os
import re
import subprocess

from common_utils import async_exec


interested_keys = {
    "Certificates fallback": "cert_fallback_time",
    "Certificates time": "cert_time",
    "U-Boot fallback": "uboot_fallback_time",
    "U-Boot time": "uboot_time",
    "Flags force_recovery": "force_recovery",
    "Flags hardware_enforce": "hardware_enforce",
    "Flags software_enforce": "software_enforce",
    "Flags recovery_boot": "recovery_boot",
    "Flags recovery_retried": "recovery_retried",
    "ROM version": "rom_version",
    "ROM U-Boot version": "rom_uboot_version",
    "FLASH0 Signed": "flash0_signed",
    "FLASH0 Locked": "flash0_locked",
}


async def get_vboot_status():  # noqa: C901
    info = {}
    info["status"] = "-1"
    info["status_text"] = "Unsupported"
    vboot_util = "/usr/local/bin/vboot-util"
    if not os.path.isfile(vboot_util):
        return info
    try:
        retcode, stdout, _ = await async_exec(["/usr/local/bin/vboot-util"])
        data = stdout.splitlines()

        info["status_text"] = data[-1].strip()
        if "Verified boot is not supported" in info["status_text"]:
            return info
        m = re.match("Status CRC: (0[xX][0-9a-fA-F]+)", data[-4].strip())
        if m:
            info["status_crc"] = m.group(1)
        m = re.match(r"Status type \((\d+)\) code \((\d+)\)", data[-2].strip())
        if m:
            info["status"] = "{}.{}".format(m.group(1), m.group(2))
        m = re.match(r"TPM.? status  \((\d+)\)", data[-3].strip())
        if m:
            info["tpm_status"] = m.group(1)
        for line in data:
            a = line.split(": ")
            if len(a) == 2:
                key = a[0].strip()
                value = a[1].strip()
                if key in interested_keys:
                    info[interested_keys[key]] = value
    except (OSError, subprocess.CalledProcessError):
        pass
    return info
