#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
# ELBERTTODO SCDCPLD

maj_ver="cpld_ver_major"
min_ver="cpld_ver_minor"
exitCode=0

echo "------SUP-FPGA------"

if [ ! -d "$SUPCPLD_SYSFS_DIR" ]; then
    echo "SUP FPGA is not detected"
else
    val_major=$(head -n 1 "$SUPCPLD_SYSFS_DIR"/"$maj_ver")
    val_minor=$(head -n 1 "$SUPCPLD_SYSFS_DIR"/"$min_ver")
    echo "SUP_FPGA: $((val_major)).$((val_minor))"
fi

echo "------SCD-FPGA------"
if [ ! -d "$SCDCPLD_SYSFS_DIR" ]; then
    echo "SCD FPGA is not detected"
    echo "Unable to retrieve PIM FPGA versions either"
else
    val_major=$(head -n 1 "$SCDCPLD_SYSFS_DIR"/"$maj_ver")
    val_minor=$(head -n 1 "$SCDCPLD_SYSFS_DIR"/"$min_ver")
    echo "SCD_FPGA: $((val_major)).$((val_minor))"

    echo "------PIM-FPGA------"
    pim_list="1 2 3 4 5 6 7 8"
    for pim in ${pim_list}; do
      pim_maj_ver_file="lc${pim}_fpga_rev_major"
      pim_min_ver_file="lc${pim}_fpga_rev_minor"
      val1=$(head -n 1 "$SCDCPLD_SYSFS_DIR"/"$pim_maj_ver_file")
      val2=$(head -n 1 "$SCDCPLD_SYSFS_DIR"/"$pim_min_ver_file")
      if [ "$((val1))" -eq 255 ]; then
        # Print all the PIM status then exit
        echo "PIM $pim: NOT DETECTED"
        exitCode=1
      else
        echo "PIM $pim : $((val1)).$((val2))"
      fi
    done
fi

if [ "$exitCode" -ne 0 ]; then
    echo "Since all the PIMs detection didn't succeed as listed above... exiting"
    exit 1
fi
