#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

from utils.cit_logger import Logger


def check_fru_availability(fru: str) -> bool:
    """[summary]check if FRU present
    we use fruid-util to check, if fru present, then expected output is:
    root@sled2401-oob:~# fruid-util slot1

    FRU Information           : Server board 1
    ---------------           : ------------------
    Chassis Type              : Rack Mount Chassis
    ....

    Args:
        fru (str): fru name, ex. riser_slot1, slot2

    Returns:
        bool: return true if fru present otherwise false
    """
    cmd = ["/usr/local/bin/fruid-util", fru]
    f = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    out, _ = f.communicate()
    if re.search("FRU Information", out.decode("utf-8")):
        return True
    return False


    return os.path.exists("/usr/local/bin/tests2/dummy_qemu")


def mac_verify(mac):
    """
    Helper method to verify if a MAC address is properly formatted
    """
    if re.match("[0-9a-f]{2}([:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$", mac.lower()):
        return True
    return False


def read_data_from_filepath(path):
    """
    Helper method to read file from a file
    """
    try:
        handle = open(path, "r")
        data = handle.readline().rstrip()
        handle.close()
        Logger.debug("Reading path={} data={}".format(path, data))
        return data
    except Exception:
        raise ("Failed to read path={}".format(path))


def running_systemd():
    return "systemd" in os.readlink("/proc/1/exe")
