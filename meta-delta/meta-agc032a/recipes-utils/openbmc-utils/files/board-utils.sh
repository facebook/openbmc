#!/bin/bash
#
# Copyright 2020-present Delta Eletronics, Inc. All Rights Reserved.
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

# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

SWPLD1_SYSFS_DIR=$(i2c_device_sysfs_abspath 7-0032)
SWPLD2_SYSFS_DIR=$(i2c_device_sysfs_abspath 7-0034)
SWPLD3_SYSFS_DIR=$(i2c_device_sysfs_abspath 7-0035)

# RESET
VR_3V3_RST_SYSFS="${SWPLD1_SYSFS_DIR}/vr_3v3_rst"
VR_0V8_RST_SYSFS="${SWPLD1_SYSFS_DIR}/vr_0v8_rst"
MAC_RST_SYSFS="${SWPLD1_SYSFS_DIR}/mac_rst"
BMC_RST_SYSFS="${SWPLD1_SYSFS_DIR}/bmc_rst"
LPC_RST_SYSFS="${SWPLD1_SYSFS_DIR}/lpc_rst"
OOB_PCIE_RST_SYSFS="${SWPLD1_SYSFS_DIR}/oob_pcie_rst"
MAC_PCIE_RST_SYSFS="${SWPLD1_SYSFS_DIR}/mac_pcie_rst"

# CPLD version
SWPLD1_BOARD_ID=$(head -n 1 "$SWPLD1_SYSFS_DIR/board_id" 2> /dev/null)
SWPLD1_BOARD_VER=$(head -n 1 "$SWPLD1_SYSFS_DIR/board_ver" 2> /dev/null)
SWPLD1_VER_TYPE=$(head -n 1 "$SWPLD1_SYSFS_DIR/swpld1_ver_type" 2> /dev/null)
SWPLD1_VER=$(head -n 1 "$SWPLD1_SYSFS_DIR/swpld1_ver" 2> /dev/null)
SWPLD2_VER_TYPE=$(head -n 1 "$SWPLD2_SYSFS_DIR/swpld2_ver_type" 2> /dev/null)
SWPLD2_VER=$(head -n 1 "$SWPLD2_SYSFS_DIR/swpld2_ver" 2> /dev/null)
SWPLD3_VER_TYPE=$(head -n 1 "$SWPLD3_SYSFS_DIR/swpld3_ver_type" 2> /dev/null)
SWPLD3_VER=$(head -n 1 "$SWPLD3_SYSFS_DIR/swpld3_ver" 2> /dev/null)

# PSU
PSU1_PRESENT=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu1_present" 2> /dev/null)
PSU2_PRESENT=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu2_present" 2> /dev/null)
PSU1_ENABLE=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu1_en" 2> /dev/null)
PSU2_ENABLE=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu2_en" 2> /dev/null)
PSU1_EEPROM_WP=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu1_eeprom_wp" 2> /dev/null)
PSU2_EEPROM_WP=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu2_eeprom_wp" 2> /dev/null)
PSU1_STATE=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu1_state" 2> /dev/null)
PSU2_STATE=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu2_state" 2> /dev/null)
PSU1_ALERT=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu1_alert" 2> /dev/null)
PSU2_ALERT=$(head -n 1 "$SWPLD1_SYSFS_DIR/psu2_alert" 2> /dev/null)



wedge_is_us_on() {
    return 0
}

wedge_board_type_rev(){
    echo "AGC032A"
}

delta_board_rev() {
    echo "$SWPLD1_BOARD_VER"
}

delta_prepare_cpld_update() {
    echo "Stop fscd service."
    sv stop fscd

    echo "Disable watchdog."
    /usr/local/bin/wdtcli stop

    echo "Set fan speed 40%."
    set_fan_speed.sh 40 all both
}