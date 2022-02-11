#!/bin/sh
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

### BEGIN INIT INFO
# Provides:          setup_bios_util
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup bios-util config
### END INIT INFO

echo -n "Setup bios-util config... "

bios_util_config_path="/usr/local/fbpackages/bios-util/bios_support.json"
bc_type5_config_path="/usr/local/fbpackages/bios-util/BIOS_UTIL_BCT5_v2.json"
bc_type7_config_path="/usr/local/fbpackages/bios-util/BIOS_UTIL_BCT7_v2.json"
IOM_M2=1      # IOM type: M.2
IOM_IOC=2     # IOM type: IOC

sh /usr/local/bin/check_pal_sku.sh > /dev/NULL
  PAL_SKU=$(($? & 0x3))
  if [ "$PAL_SKU" == "$IOM_M2" ]; then
    cp ${bc_type5_config_path} ${bios_util_config_path}
  elif [ "$PAL_SKU" == "$IOM_IOC" ]; then
  	cp ${bc_type7_config_path} ${bios_util_config_path}
  else
  	echo "Setup bios-util: System type is unknown! Please confirm the system type."
  fi

echo "done."
