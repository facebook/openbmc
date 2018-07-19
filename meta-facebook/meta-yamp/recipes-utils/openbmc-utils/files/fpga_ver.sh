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

maj_ver="cpld_ver_major"
min_ver="cpld_ver_minor"
sup_fpga="/sys/bus/i2c/drivers/supcpld/12-0043/"
scd_fpga="/sys/bus/i2c/drivers/scdcpld/4-0023/"


echo "------SUP-FPGA------"

if [ ! -d $sup_fpga ]; then
    echo "SUP FPGA is not detected"
else
    val_major=$(head -n 1 $sup_fpga/$maj_ver)
    val_minor=$(head -n 1 $sup_fpga/$min_ver)
    echo "SUP_FPGA: $(($val_major)).$(($val_minor))"
fi

echo "------SCD-FPGA------"
if [ ! -d $scd_fpga ]; then
    echo "SCD FPGA is not detected"
    echo "Unable to retrieve PIM FPGA versions eihter"
else
    val_major=$(head -n 1 $scd_fpga/$maj_ver)
    val_minor=$(head -n 1 $scd_fpga/$min_ver)
    echo "SCD_FPGA: $(($val_major)).$(($val_minor))"

    echo "------PIM-FPGA------"
    pim_list="1 2 3 4 5 6 7 8"
    for pim in ${pim_list}; do
      pim_ver_file="lc${pim}_fpga_revision"
      val=$(head -n 1 $scd_fpga/$pim_ver_file)
      if [ "${val}" == "0x0" ]; then
        echo "PIM $pim : NOT_DETECTED"
      else
        echo "PIM $pim : $val"
      fi
    done
fi

