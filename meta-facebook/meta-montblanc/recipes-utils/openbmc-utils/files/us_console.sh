#!/bin/sh
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

usage() {
    echo "$0 <connect | disconnect>"
}

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

uart_sel="$(i2c_device_sysfs_abspath 1-0035)/uart_select"

if [ $# -ne 1 ]; then
    usage
    exit 1
fi

if [ "$1" = "connect" ]; then
    # ignore error code when run with qemu
    echo 0x0 > "$uart_sel" || exit 0
elif [ "$1" = "disconnect" ]; then
    # ignore error code when run with qemu
    echo 0x1 > "$uart_sel" || exit 0
else
    usage
    exit 1
fi
