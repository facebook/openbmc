#!/bin/bash
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

. /usr/local/bin/openbmc-utils.sh

logger "Reset YAMP Forwarding ASIC"
SCDCPLD_SYSFS_DIR="/sys/class/i2c-dev/i2c-4/device/4-0023"
SCD_TH3_RST_ON_SYSFS="${SCDCPLD_SYSFS_DIR}/th3_reset"
SCD_TH3_PCI_RST_ON_SYSFS="${SCDCPLD_SYSFS_DIR}/th3_pci_reset"
logger "Putting TH3 into reset first"
echo "Putting TH3 into reset first"
echo 1 > $SCD_TH3_RST_ON_SYSFS
echo 1 > $SCD_TH3_PCI_RST_ON_SYSFS
sleep 1
logger "Taking TH3 sys out of reset"
echo "Taking TH3 sys out of reset"
echo 0 > $SCD_TH3_RST_ON_SYSFS
sleep 1
logger "Taking TH3 pci out of reset"
echo "Taking TH3 pci out of reset"
echo 0 > $SCD_TH3_PCI_RST_ON_SYSFS
sleep 1
