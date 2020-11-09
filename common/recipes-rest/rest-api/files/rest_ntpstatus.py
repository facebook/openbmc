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

from subprocess import check_output

from rest_utils import running_systemd


def get_ntpq_stats():
    try:
        cmd = "/usr/sbin/ntpq -6 -p"
        data = check_output(cmd, shell=True)
        data = data.decode()
        ntpstats = {}
        for line in data.splitlines():
            # Only care about system peer
            if not line.startswith("*"):
                continue

            parts = line[1:].split()
            ntpstats["stratum"] = float(parts[2])
            ntpstats["when"] = float(parts[4])
            ntpstats["poll"] = float(parts[5])
            ntpstats["reach"] = float(parts[6])
            ntpstats["delay"] = float(parts[7])
            ntpstats["offset"] = float(parts[8])
            ntpstats["jitter"] = float(parts[9])
            return {
                "data": {"Information": ntpstats, "Actions": [], "Resources": []},
                "status": 200,
            }
        return {
            "data": {"Information": {"reason": data}, "Actions": [], "Resources": []},
            "status": 404,
        }
    except Exception as ex:
        return {
            "data": {
                "Information": {"reason": repr(ex)},
                "Actions": [],
                "Resources": [],
            },
            "status": 500,
        }


def get_ntp_stats():
    if running_systemd():
        return fudge_timedatectl_stats()
    else:
        return get_ntpq_stats()


def fudge_timedatectl_stats():
    # timedatectl on rocko displays far less information than ntpq. We
    # don't have show-timesync either. We print some fake data that
    # looks alarming if synchronization is bad and looks sane if time
    # is correctly synchronized.
    cmd = "/usr/bin/timedatectl"
    try:
        data = check_output(cmd.split())

        data = data.decode()

        # Default bad statistics, pointing that there is no real
        # synchronization...
        ntpstats = {"stratum": 16, "delay": 5000, "jitter": 5000, "offset": 5000}

        for line in data.splitlines():
            # Handle differences in whitespaces...
            if "NTP synchronized: yes" in line:
                ntpstats = {
                    "delay": 50,
                    "jitter": 50,
                    "stratum": 2,
                    "offset": 50,
                }
                return {
                    "data": {"Information": ntpstats, "Actions": [], "Resources": []},
                    "status": 200,
                }
        return {
            "data": {"Information": {"reason": data}, "Actions": [], "Resources": []},
            "status": 404,
        }
    except Exception as ex:
        return {
            "data": {
                "Information": {"reason": repr(ex)},
                "Actions": [],
                "Resources": [],
            },
            "status": 500,
        }
