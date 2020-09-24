#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# This utility displays the inserted PIM types

pim_list="2 3 4 5 6 7 8 9"
for pim in ${pim_list}; do
    pim_present=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_present)
    if [ "$((pim_present))" -eq 0 ]; then
        echo "PIM $pim: NOT INSERTED"
    else
        pim_type='NOT DETECTED'
        fru="$(peutil "$pim" 2>&1)"
        if echo "$fru" | grep -q '88-16CD'; then
            pim_type='PIM16Q'
        elif echo "$fru" | grep -q '88-8D'; then
            pim_type='PIM8DDM'
        fi
        echo "PIM $pim: $pim_type"
    fi
done
