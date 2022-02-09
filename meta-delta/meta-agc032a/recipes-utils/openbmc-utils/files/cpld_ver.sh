#!/bin/bash
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

. /usr/local/bin/openbmc-utils.sh

echo "SMB_SYSCPLD: $((SMB_CPLD_VER)).$((SMB_CPLD_SUB_VER))"
echo "SMB_PWRCPLD: $((PWR_CPLD_VER)).$((PWR_CPLD_SUB_VER))"
echo "SCMCPLD: $((SCM_CPLD_VER)).$((SCM_CPLD_SUB_VER))"
echo "FCMCPLD: $((FCM_CPLD_VER)).$((FCM_CPLD_SUB_VER))"
