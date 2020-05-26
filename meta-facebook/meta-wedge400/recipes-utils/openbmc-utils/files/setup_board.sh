#!/bin/sh
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

### BEGIN INIT INFO
# Provides:          setup_board.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup the board
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

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

# Read board type from Board type GPIO pin and save to cache
brd_type=$(wedge_board_type)
if [ ! -d /tmp/cache_store ]; then
  mkdir /tmp/cache_store
fi
echo "$brd_type" > /tmp/cache_store/board_type

# Check board type to select lmsensor configuration
if [ $((brd_type)) -eq 0 ]; then
  cp /etc/sensors.d/custom/wedge400.conf /etc/sensors.d/wedge400.conf
elif  [ $((brd_type)) -eq 1 ]; then
  cp /etc/sensors.d/custom/wedge400c.conf /etc/sensors.d/wedge400c.conf
fi

# export_gpio_pin for PCA9555 4-0027
gpiocli export -c 4-0027 -o 8 --shadow ISO_DEBUG_RST_BTN_N
gpiocli export -c 4-0027 -o 9 --shadow ISO_PWR_BTN_N
gpiocli export -c 4-0027 -o 10 --shadow PWRGD_PCH_PWROK
gpiocli export -c 4-0027 -o 11 --shadow ISO_CB_RESET_N
gpiocli export -c 4-0027 -o 12 --shadow COM_PWROK
gpiocli export -c 4-0027 -o 13 --shadow CPU_CATERR_MSMI
gpiocli export -c 4-0027 -o 14 --shadow ISO_COM_SUS_S3_N
gpiocli export -c 4-0027 -o 15 --shadow DEBUG_UART_SELECT
