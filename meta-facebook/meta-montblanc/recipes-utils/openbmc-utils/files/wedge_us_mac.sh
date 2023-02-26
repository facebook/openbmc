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

# Get output from seutil
seutil_output=$(seutil | grep 'Local MAC:')

# Check if the read was successful and output matches with our format
match=$(echo "$seutil_output" | grep -c 'Local MAC: [0-9a-f][0-9a-f]:')
if [ "$match" = "1" ]; then
    # Mac address: xx:xx:xx:xx:xx:xx
    echo "$seutil_output" | cut -d ' ' -f 3
    exit 0
else
    echo "Cannot find out the microserver MAC" 1>&2
    exit 1
fi

