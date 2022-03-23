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

### BEGIN INIT INFO
# Provides:          setup_board.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup the board
### END INIT INFO

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Enable the isolation buffer between BMC and COMe i2c bus
echo 0 > "${SCMCPLD_SYSFS_DIR}/com_exp_pwr_enable"

# Make the backup BIOS flash connect to COMe instead of BMC
#echo 0 > ${SCMCPLD_SYSFS_DIR}/com_spi_oe_n
#echo 0 > ${SCMCPLD_SYSFS_DIR}/com_spi_sel

# Setup management port LED
/usr/local/bin/setup_mgmt.sh led &

# Setup USB Serial to RS485
echo 0 > "${SMBCPLD_SYSFS_DIR}/cpld_usb_mux_sel_0"
echo 0 > "${SMBCPLD_SYSFS_DIR}/cpld_usb_mux_sel_1"

# Init server_por_cfg
#if [ ! -f "/mnt/data/kv_store/server_por_cfg" ]; then
#    /usr/bin/kv set server_por_cfg on persistent
#fi

# Force COMe's UART connect to BMC UART-5 and FB USB debug
# UART connect to BMC UART-2
echo 0x3 > "$SMBCPLD_SYSFS_DIR/uart_selection"

# Read board type from SMB CPLD and keep in cache store
brd_type=$(head -n 1 "$SMBCPLD_SYSFS_DIR/board_type")
if [ ! -d /tmp/cache_store ]; then
  mkdir /tmp/cache_store
fi
echo "$brd_type" > /tmp/cache_store/board_type


cp /etc/sensors.d/custom/fuji.conf /etc/sensors.d/fuji.conf

kv set smb_update_flag 1
kv set scm_update_flag 1
kv set pim1_update_flag 1
kv set pim2_update_flag 1
kv set pim3_update_flag 1
kv set pim4_update_flag 1
kv set pim5_update_flag 1
kv set pim6_update_flag 1
kv set pim7_update_flag 1
kv set pim8_update_flag 1
kv set psu1_update_flag 1
kv set psu2_update_flag 1
kv set psu3_update_flag 1
kv set psu4_update_flag 1

# export_gpio_pin for PCA9534 54-0021
gpiocli export -c 54-0021 -o 0 --shadow PDB_L_JTAG_TDO
gpiocli export -c 54-0021 -o 1 --shadow PDB_L_JTAG_TDI
gpiocli export -c 54-0021 -o 2 --shadow PDB_L_JTAG_TMS
gpiocli export -c 54-0021 -o 3 --shadow PDB_L_JTAG_TCK
gpiocli export -c 54-0021 -o 4 --shadow PDB_L_JTAG_PGM
gpiocli export -c 54-0021 -o 7 --shadow PDB_L_HITLESS
gpiocli export -c 62-0021 -o 0 --shadow PDB_R_JTAG_TDO
gpiocli export -c 62-0021 -o 1 --shadow PDB_R_JTAG_TDI
gpiocli export -c 62-0021 -o 2 --shadow PDB_R_JTAG_TMS
gpiocli export -c 62-0021 -o 3 --shadow PDB_R_JTAG_TCK
gpiocli export -c 62-0021 -o 4 --shadow PDB_R_JTAG_PGM
gpiocli export -c 62-0021 -o 7 --shadow PDB_R_HITLESS

