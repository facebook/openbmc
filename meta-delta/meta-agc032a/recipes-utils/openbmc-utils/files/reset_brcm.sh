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

# There are two reset pins to ASIC. System reset and PCI reset.
#
# When PWR_TH_RST_SYSFS is set to 0, CPLD will drive both reset pins to trigger
# reset.
#
# When PWR_TH_RST_SYSFS is set to 1, CPLD will drive system reset, followed by
# PCI reset following the reset sequeence.
#
# There is another sysfs file, 'cpld_mac_pcie_perst_l', which can be used
# to control the PCI reset pin separately. However, we don't use it for now
# as we depend on the CPLD for the correct reset sequence.

echo 0 > $PWR_TH_RST_SYSFS
sleep 1
echo 1 > $PWR_TH_RST_SYSFS

logger "Reset Tomahawk"
