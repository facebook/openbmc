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

# Minimal seed: covers the canary failure mode (REST API not responding at
# all). Owners: extend with platform-specific endpoints (/api/spb, /api/sys,
# /api/sys/server*/*, etc.) as needed -- the QEMU-aware test harness in
# test_rest_endpoint.py limits the QEMU run to /api regardless, so adding
# more endpoints only affects the on-hardware run.
REST_END_POINTS = {
    "/api": ["Description", "version"],
}
