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
            file://patches/0503-fix-kernel-panic-during-i2c-transaction-timeout.patch \
            file://patches/0510-driver-xdpe132g5c-pmbus.patch \
            file://patches/0511-gpio-aspeed-add-dummy-read-to-ensure-the-write-complete.patch \
           "

#
# Include fuji-specific patches
#
SRC_URI += "file://1001-ARM-dts-aspeed-fuji-add-i2c-slave-timeout-for-i2c-bus.patch \
	    file://1002-fuji-remove-adm1278-from-dts-and-let-pro.patch \
	    file://1003-ARM-dts-aspeed-fuji-enable-jtag-Spi.patch \
	    file://1011-hwmon-add-net_brcm-driver.patch \
	    file://1021-hwmon-mp2975-add-compatible-for-mp2978-1025.patch \
	    file://1031-Added-Facebook-mfd-usmc-driver.patch \
	    file://1032-Added-Facebook-fboss-usmc-driver.patch \
	    file://1033-Include-spidev-into-SPI-ID-table.patch \
           "

