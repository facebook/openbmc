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
import json
import os
import re
import subprocess

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd

run_test_dir = "/run/tests2/"
usr_test_dir = "/usr/local/bin/tests2"


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


def check_board_product(fru: str = None, product: str = None) -> bool:
    """[summary] check if "Board Product" equals to the give product

    Args:
        product (str): product name

    Returns:
        bool: [description]
    """
    cmd = ["/usr/local/bin/fruid-util", fru, "--json"]
    f = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    out, _ = f.communicate()
    out_dict = json.loads(out.decode("utf-8"))[0]
    return out_dict["Board Product"] == product


def qemu_check():
    if os.path.exists(run_test_dir):
        return os.path.exists("{}/dummy_qemu".format(run_test_dir))
    else:
        return os.path.exists("{}/dummy_qemu".format(usr_test_dir))


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


def tests_dir():
    if os.path.exists(run_test_dir):
        return "{}/tests/".format(run_test_dir)
    else:
        return "{}/tests/".format(usr_test_dir)


# TODO: Stop gap solution. After D45972779 commit, this will be replaced
# by tests_dir()
def fscd_test_data_dir(platform):
    if not os.path.exists("{}/tests/".format(run_test_dir)):
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/userver".format(run_test_dir, platform)
        )
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/switch".format(run_test_dir, platform)
        )
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/intake".format(run_test_dir, platform)
        )
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/outlet".format(run_test_dir, platform)
        )
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/json-max".format(
                run_test_dir, platform
            )
        )
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/th3".format(run_test_dir, platform)
        )
        run_shell_cmd(
            "mkdir -p {}/tests/{}/test_data/fscd/inlet".format(run_test_dir, platform)
        )
    return "{}/tests/".format(run_test_dir)


# TODO: Stop gap solution. After D45972779 commit, this will be replaced
# by tests_dir()
def fscd_config_dir():
    if os.path.exists("{}/cit_runner.py".format(run_test_dir)):
        return "{}/tests/".format(run_test_dir)
    else:
        return "{}/tests/".format(usr_test_dir)
