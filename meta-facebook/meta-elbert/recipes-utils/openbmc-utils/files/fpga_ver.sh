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

# Print "<label>: <major>.<minor>" for one CPLD.
#
# Reads must not write to stderr. Callers run this script as
#   fpga_ver.sh | grep <LABEL> | cut -f 2 -d ' '
# and stderr bypasses that pipeline: it reaches the terminal immediately
# while the pipeline block-buffers stdout until the script exits. One
# unreadable register therefore prints ahead of every other component's
# version and is read as if it were that version. Report the failure on
# stdout, on the component's own line, where the grep will scope it.
print_cpld_ver() {
    local label="$1" major minor
    major=$(head -n 1 "$2" 2>/dev/null)
    minor=$(head -n 1 "$3" 2>/dev/null)
    if [ -z "$major" ] || [ -z "$minor" ]; then
        echo "$label: VERSION_READ_ERROR"
        return 1
    fi
    echo "$label: $((major)).$((minor))"
}

echo "------SCM-FPGA------"

if [ ! -d "$SCMCPLD_SYSFS_DIR" ]; then
    echo "SCM_FPGA: FPGA_DRIVER_NOT_DETECTED"
    exitCode=1
else
    print_cpld_ver "SCM_FPGA" "$SCMCPLD_SYSFS_DIR"/"$maj_ver" "$SCMCPLD_SYSFS_DIR"/"$min_ver" || exitCode=1
fi

echo "------FAN-FPGA------"
if [ ! -d "$FANCPLD_SYSFS_DIR" ]; then
    echo "FAN_FPGA: FPGA_DRIVER_NOT_DETECTED"
    exitCode=1
else
    print_cpld_ver "FAN_FPGA" "$FANCPLD_SYSFS_DIR"/"$maj_ver" "$FANCPLD_SYSFS_DIR"/"$min_ver" || exitCode=1
fi

echo "------SMB-FPGA------"
if [ ! -d "$SMBCPLD_SYSFS_DIR" ]; then
    echo "SMB_FPGA: FPGA_DRIVER_NOT_DETECTED"
    echo "Unable to retrieve PIM FPGA versions either"
    exitCode=1
else
    print_cpld_ver "SMB_FPGA" "$SMBCPLD_SYSFS_DIR"/"$maj_ver" "$SMBCPLD_SYSFS_DIR"/"$min_ver" || exitCode=1

    echo "------SMB-CPLD------"
    print_cpld_ver "SMB_CPLD" "$SMBCPLD_SYSFS_DIR"/th4_cpld_ver_major \
        "$SMBCPLD_SYSFS_DIR"/th4_cpld_ver_minor || exitCode=1

    echo "------PIM-FPGA------"
    pim_list="2 3 4 5 6 7 8 9"
    for pim in ${pim_list}; do
      pim_present=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_present 2>/dev/null)
      pim_major=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_fpga_rev_major 2>/dev/null)
      pim_minor=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_fpga_rev_minor 2>/dev/null)
      if [ "$((pim_present))" -eq 0 ]; then
        echo "PIM $pim: NOT_INSERTED"
      elif [ "$((pim_major))" -eq 255 ]; then
        # The FPGA version read was 0xFF, which indicates not detected
        echo "PIM $pim: VERSION_NOT_DETECTED"
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
