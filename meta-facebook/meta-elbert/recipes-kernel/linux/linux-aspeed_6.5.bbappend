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

FILESEXTRAPATHS:prepend := "${THISDIR}/patches_6.5:"

#
# Include patches from common/recipes-kernel.
#
SRC_URI += "file://0500-hwmon-Add-net_brcm-driver.patch \
           "

#
# Include patches from elbert machine layer.
#
SRC_URI += "file://1001-ARM-dts-aspeed-elbert-Enable-spi1-controller.patch \
            file://1002-ARM-dts-aspeed-elbert-Enable-jtag1-controller.patch \
           "
