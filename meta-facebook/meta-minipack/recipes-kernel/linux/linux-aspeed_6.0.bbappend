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
           "
#
# Include minipack-specific patches
#
SRC_URI += "file://1001-ARM-dts-aspeed-minipack-set-i2c0-slave-inactive-time.patch \
            file://1002-ARM-dts-minipack-aspeed-kernel-6.0-add-the-spi-flash.patch \
           "
