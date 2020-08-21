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

maj_ver="cpld_ver_major"
min_ver="cpld_ver_minor"
exitCode=0

echo "------SCM-FPGA------"

if [ ! -d "$SCMCPLD_SYSFS_DIR" ]; then
    echo "SCM_FPGA: FPGA DRIVER NOT DETECTED"
    exitCode=1
else
    val_major=$(head -n 1 "$SCMCPLD_SYSFS_DIR"/"$maj_ver")
    val_minor=$(head -n 1 "$SCMCPLD_SYSFS_DIR"/"$min_ver")
    echo "SCM_FPGA: $((val_major)).$((val_minor))"
fi

echo "------FAN-FPGA------"
if [ ! -d "$FANCPLD_SYSFS_DIR" ]; then
    echo "FAN_FPGA: FPGA DRIVER NOT DETECTED"
    exitCode=1
else
    val_major=$(head -n 1 "$FANCPLD_SYSFS_DIR"/"$maj_ver")
    val_minor=$(head -n 1 "$FANCPLD_SYSFS_DIR"/"$min_ver")
    echo "FAN_FPGA: $((val_major)).$((val_minor))"
fi

echo "------SMB-FPGA------"
if [ ! -d "$SMBCPLD_SYSFS_DIR" ]; then
    echo "SMB_FPGA: FPGA DRIVER NOT DETECTED"
    echo "Unable to retrieve PIM FPGA versions either"
    exitCode=1
else
    val_major=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/"$maj_ver")
    val_minor=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/"$min_ver")
    echo "SMB_FPGA: $((val_major)).$((val_minor))"

    echo "------SMB-CPLD------"
    val_major=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/th4_cpld_ver_major)
    val_minor=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/th4_cpld_ver_minor)
    echo "SMB_CPLD: $((val_major)).$((val_minor))"

    echo "------PIM-FPGA------"
    pim_list="2 3 4 5 6 7 8 9"
    for pim in ${pim_list}; do
      pim_present=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_present)
      pim_major=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_fpga_rev_major)
      pim_minor=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_fpga_rev_minor)
      if [ "$((pim_present))" -eq 0 ]; then
        echo "PIM $pim: NOT INSERTED"
      elif [ "$((pim_major))" -eq 255 ]; then
        # The FPGA version read was 0xFF, which indicates not detected
        echo "PIM $pim: VERSION NOT DETECTED"
        exitCode=1
      else
        echo "PIM $pim: $((pim_major)).$((pim_minor))"
      fi
    done
fi

if [ "$exitCode" -ne 0 ]; then
    echo "One or more fpga versions was not detected... exiting"
    exit 1
fi
