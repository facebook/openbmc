#!/usr/bin/env python3
#
# Copyright 2016-present Facebook. All Rights Reserved.
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
import subprocess
import syslog


SCM_CPLD_PATH = "/usr/local/bin/scm_cpld_rev.sh"
SYS_CPLD_VERSION = "/usr/local/bin/cpld_rev.sh"
QSFP_CPLD_VERSION = "/usr/local/bin/qsfp_cpld_ver.sh"


def _firmware_json(ver, sub_ver, int_base=16):
    return {"version": int(ver, int_base), "sub_version": int(sub_ver, int_base)}


def _get_qsfp_cpld_info(cmd):
    ver, sub_ver = 0, 0
    try:
        data = subprocess.check_output([cmd])
        data = data.decode()
        ver, sub_ver = data.split(",")
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, "Error getting CPLD versions : {}".format(e))

    return _firmware_json(ver, sub_ver)


def _get_cpld_info(cmd):
    ver, sub_ver = 0, 0
    try:
        data = subprocess.check_output([cmd])
        data = data.decode()
        ver, sub_ver = data.split(".")
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, "Error getting CPLD versions : {}".format(e))

    return _firmware_json(ver, sub_ver, int_base=10)


def get_firmware_info():
    return {
        "SYS_CPLD": _get_cpld_info(cmd=SYS_CPLD_VERSION),
        "SCM_CPLD": _get_cpld_info(cmd=SCM_CPLD_PATH),
        "QSFP_CPLD": _get_qsfp_cpld_info(cmd=QSFP_CPLD_VERSION),
    }


if __name__ == "__main__":
    print("{}".format(get_firmware_info()))
