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

from rest_utils import DEFAULT_TIMEOUT_SEC


def get_mTerm_status():
    result = []
    proc = subprocess.Popen(
        ["ps | grep mTerm"], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    try:
        data, err = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    except proc.TimeoutError as ex:
        data = ex.output
        err = ex.error

    is_running = "Not Running"
    if b"mTerm_server" in data:
        is_running = "Running"

    result = {
        "Information": {"mTerm Server Status": is_running},
        "Actions": [],
        "Resources": [],
    }
    return result
