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
brd_rev=$(wedge_board_rev)

#Read PSU type from eeprom and save to cache
pwr_type=$(wedge_power_supply_type)

/usr/bin/kv set "board_type" "$brd_type"
/usr/bin/kv set "power_type" "$pwr_type"

# Check board type to select lmsensor configuration
if [ $((brd_type)) -eq 0 ]; then
  cp /etc/sensors.d/custom/wedge400.conf /etc/sensors.d/wedge400.conf
  # Check board revision
  if [ $((brd_rev)) -ge 6 ]; then # for respin or newer
    cp /etc/sensors.d/custom/wedge400-respin.conf /etc/sensors.d/wedge400-respin.conf
  else
    cp /etc/sensors.d/custom/wedge400-evt-mp.conf /etc/sensors.d/wedge400-evt-mp.conf
  fi;
elif  [ $((brd_type)) -eq 1 ]; then
  cp /etc/sensors.d/custom/wedge400c.conf /etc/sensors.d/wedge400c.conf
  # Check board revision
  if [ $((brd_rev)) -ge 4 ]; then # for respin or newer
    cp /etc/sensors.d/custom/wedge400c-respin.conf /etc/sensors.d/wedge400c-respin.conf
  else
    cp /etc/sensors.d/custom/wedge400c-evt-dvt2.conf /etc/sensors.d/wedge400c-evt-dvt2.conf
  fi;
fi

# For the time being, we set GB VDD Core to 825mv, to take care of
# some of the early DVT W400c devices whose VRD default value upon power-up
# is NOT 825. This will not harm newer devices with correct default value.
if [ $((brd_type)) -eq 1 ]; then
  echo "Running GB VDD Core fix routine."
  /usr/local/bin/set_vdd.sh 825
fi

# export_gpio_pin for PCA9555 4-0027
gpiocli export -c 4-0027 -o 8 --shadow ISO_DEBUG_RST_BTN_N
gpiocli export -c 4-0027 -o 9 --shadow ISO_PWR_BTN_N
gpiocli export -c 4-0027 -o 10 --shadow PWRGD_PCH_PWROK
gpiocli export -c 4-0027 -o 11 --shadow ISO_CB_RESET_N
gpiocli export -c 4-0027 -o 12 --shadow COM_PWROK
gpiocli export -c 4-0027 -o 13 --shadow CPU_CATERR_MSMI_EXT
gpiocli export -c 4-0027 -o 14 --shadow ISO_COM_SUS_S3_N
gpiocli export -c 4-0027 -o 15 --shadow DEBUG_UART_SELECT
