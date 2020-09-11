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
import subprocess

from aiohttp.log import server_logger


GPIOCLI_CMD = "/usr/local/bin/gpiocli"


def read_gpio_by_name(name: str, chip: str = "aspeed-gpio") -> int:
    try:
        p = subprocess.Popen(
            [GPIOCLI_CMD, "--chip", chip, "--pin-name", name, "get-value"],
            stdout=subprocess.PIPE,
        )
        out, err = p.communicate()
        out = out.decode()
        return int(out.split("=")[1])
    except Exception as exc:
        server_logger.exception("Error getting gpio value %s " % name, exc_info=exc)
        return None


def read_gpio_by_shadow(shadow: str) -> int:
    try:
        p = subprocess.Popen(
            [GPIOCLI_CMD, "--shadow", shadow, "get-value"], stdout=subprocess.PIPE
        )
        out, err = p.communicate()
        out = out.decode()
        return int(out.split("=")[1])
    except Exception as exc:
        server_logger.exception("Error getting gpio value %s " % shadow, exc_info=exc)
        return None


def get_wedge_slot():
    p = subprocess.Popen(
        "source /usr/local/bin/openbmc-utils.sh;" "wedge_slot_id $(wedge_board_type)",
        shell=True,
        stdout=subprocess.PIPE,
    )
    out, err = p.communicate()
    out = out.decode()
    try:
        slot = int(out.strip("\n"))
    except:
        slot = 0
    return slot
