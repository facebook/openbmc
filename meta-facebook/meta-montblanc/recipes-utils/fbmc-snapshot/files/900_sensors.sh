#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

echo -e "\n##### SENSOR DUMP #####"
netlake_identify > /dev/null 2>&1
netlake_type=$?
if [ "$netlake_type" -eq 1 ]; then
    # Netlake 2.0
    echo "APML interface is not supported. Skipping sensor dump."
elif [ "$netlake_type" -eq 0 ]; then
    # Netlake 1.0
    sensors
else
    echo "Error: Failed to identify Netlake version. Exit code: $netlake_type" >&2
    exit 1
fi
