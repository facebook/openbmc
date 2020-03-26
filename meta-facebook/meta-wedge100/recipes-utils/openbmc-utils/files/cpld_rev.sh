#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
set -e
set -o pipefail

. /usr/local/bin/openbmc-utils.sh

BASEDIR=${SYSCPLD_SYSFS_DIR}
if [[ $# -gt 0 && "$1" != "--fan" ]]; then
    echo "Prints SYS CPLD version information when called with no arguments" >&2
    echo "Prints FAN CPLD version if called with --fan" >&2
    echo "Usage: $0 [--fan]" >&2
    exit 2
fi
if [[ "$1" == "--fan" ]]; then
       BASEDIR=${FANCPLD_SYSFS_DIR}
fi
rev=$(head -n 1 ${BASEDIR}/cpld_rev)
sub_rev=$(head -n 1 ${BASEDIR}/cpld_sub_rev)

echo $(($rev)).$(($sub_rev))
