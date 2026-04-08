# Copyright 2026-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/patches_6.18:"

#
# Include patches from meru machine layer.
#
SRC_URI:append = " \
    file://1001-mtd-spi-nor-issi-add-support-for-IS25LP512MG.patch \
    file://1002-mtd-spi-nor-gigadevice-add-support-for-GD25B512.patch \
"
