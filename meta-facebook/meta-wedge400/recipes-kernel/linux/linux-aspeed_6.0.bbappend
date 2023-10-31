#Copyright 2022-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/patches:"

#
# Include patch files from meta-aspeed layer
#
SRC_URI += "file://patches/0501-i2c-add-a-slave-backend-to-receive-and-q.patch \
            file://patches/0502-add-i2c-slave-inactive-timeout-support.patch \
            file://patches/0510-driver-xdpe132g5c-pmbus.patch \
           "

#
# Include wedge400-specific patches
#
SRC_URI += "file://1001-ARM-dts-aspeed-wedge400-enable-jtag-controller.patch \
            file://1002-enable-adc5-adc8-in-wedge400.dts.patch \
            file://1003-ARM-dts-aspeed-wedge400-add-i2c-slave-timeout-for-i2c.patch \
            file://1004-ARM-dts-aspeed-wedge400-set-EMMC-max-frequency-to-25.patch \
            file://1005-ARM-dts-aspeed-wedge400-enable-aspeed-spi-controller.patch \
            file://1010-hwmon-add-net_casic-driver-1022.patch \
            file://1011-powr1220-updating-the-max_channel-to-14.patch \
            file://1022-hwmon-pmbus-pxe1610-support-pxe1211-chip.patch \
            file://1023-hwmon-pmbus-add-ir35215-driver.patch \
            file://1024-enlarge-data0-partition-8MB-to-64MB.patch \
           "
