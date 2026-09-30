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
FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI:append = " \
    file://0000-jtag-aspeed-add-AST2700-support.patch \
    file://0001-serial-8250-aspeed-add-AST2700-UART-and-UDMA-support.patch \
    file://0002-soc-aspeed-add-AST2700-UART-routing-support.patch \
    file://0003-arm64-dts-aspeed-update-AST2700-platform-peripherals.patch \
    file://0004-pinctrl-aspeed-g7-add-split-UART-groups.patch \
    file://0005-arm64-dts-aspeed-add-arista-aristabmc-bmc-device-tree.patch \
    "
