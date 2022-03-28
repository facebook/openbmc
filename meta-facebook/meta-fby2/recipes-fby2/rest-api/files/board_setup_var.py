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

fruid_map = {
    "slot1",
    "slot2",
    "slot3",
    "slot4",
    "spb",
    "nic"
}

fw_map = {
    "slot1": {
        "Bridge-IC Version":"bic_version"
    },
    "slot2": {
        "Bridge-IC Version":"bic_version"
    },
    "slot3": {
        "Bridge-IC Version":"bic_version"
    },
    "slot4": {
        "Bridge-IC Version":"bic_version"
    },
    "bmc": {
        "BMC Version":"bmc_ver",
        "BMC CPLD Version":"bmc_cpld_ver"
    }
}
