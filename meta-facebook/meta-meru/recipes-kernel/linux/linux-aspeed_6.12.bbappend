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

FILESEXTRAPATHS:prepend := "${THISDIR}/patches_6.12:"

#
# Include patches from elbert machine layer.
#
SRC_URI:append = " \
    file://1001-mtd-spi-nor-issi-add-support-for-IS25LP512MG.patch \
    file://1002-mtd-spi-nor-gigadevice-add-support-for-GD25B512.patch \
"

#
# Remove the patch (in common/recipes-kernel/) explicitly to force 8MB
# data0 partition on meru800bia and meru800bfa.
# We should delete below patch file from common layer when moving to
# kernel 6.18, because the official fix has been upstreamed:
#  - https://lists.ozlabs.org/pipermail/linux-aspeed/2025-July/015932.html
SRC_URI:remove = "file://0003-ARM-dts-aspeed-Expand-data0-partition-in-facebook-bm.patch"
