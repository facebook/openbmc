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

source /usr/local/bin/openbmc-utils.sh

maj_ver="cpld_ver_major"
min_ver="cpld_ver_minor"

echo "------SUP-FPGA------"

if [ ! -d $SUPCPLD_SYSFS_DIR ]; then
    echo "SUP FPGA is not detected"
else
    val_major=$(head -n 1 $SUPCPLD_SYSFS_DIR/$maj_ver)
    val_minor=$(head -n 1 $SUPCPLD_SYSFS_DIR/$min_ver)
    echo "SUP_FPGA: $(($val_major)).$(($val_minor))"
fi

echo "------SCD-FPGA------"
if [ ! -d $SCDCPLD_SYSFS_DIR ]; then
    echo "SCD FPGA is not detected"
    echo "Unable to retrieve PIM FPGA versions eihter"
else
    val_major=$(head -n 1 $SCDCPLD_SYSFS_DIR/$maj_ver)
    val_minor=$(head -n 1 $SCDCPLD_SYSFS_DIR/$min_ver)
    echo "SCD_FPGA: $(($val_major)).$(($val_minor))"

    echo "------PIM-FPGA------"
    pim_list="1 2 3 4 5 6 7 8"
    for pim in ${pim_list}; do
      pim_ver_file="lc${pim}_fpga_revision"
      val=$(head -n 1 $SCDCPLD_SYSFS_DIR/$pim_ver_file)
      if [ "${val}" == "0x0" ]; then
        echo "PIM $pim : NOT_DETECTED"
      else
        echo "PIM $pim : $val"
      fi
    done
fi
