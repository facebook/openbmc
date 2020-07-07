#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

board_routes = [
    "/api/sys/seutil",
    "/api/sys/firmware_info",
    "/api/sys/firmware_info/cpld",
    "/api/sys/firmware_info/fpga",
    "/api/sys/firmware_info/scm",
    "/api/sys/firmware_info/all",
    "/api/sys/presence",
    "/api/sys/presence/scm",
    "/api/sys/presence/fan",
    "/api/sys/presence/psu",
    "/api/sys/feutil",
    "/api/sys/feutil/all",
    "/api/sys/feutil/fcm",
    "/api/sys/feutil/fan1",
    "/api/sys/feutil/fan2",
    "/api/sys/feutil/fan3",
    "/api/sys/feutil/fan4",
    "/api/sys/sensors/scm",
    "/api/sys/sensors/smb",
    "/api/sys/sensors/pem1",
    "/api/sys/sensors/pem2",
    "/api/sys/sensors/psu1",
    "/api/sys/sensors/psu2",
    "/api/sys/vddcore",
    "/api/sys/vddcore/{volt}",
    "/api/sys/switch_reset",
    "/api/sys/switch_reset/cycle_reset",
    "/api/sys/switch_reset/only_reset",
    "/api/sys/gb_freq",
]
