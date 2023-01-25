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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += "\
    file://bmc_board_rev.sh \
    file://board-utils.sh \
    file://oob-mdio-util.sh \
    file://setup-gpio.sh \
    file://setup_board.sh \
    "

OPENBMC_UTILS_FILES += " \
    bmc_board_rev.sh \
    oob-mdio-util.sh \
    "

# Not needed for fbdarwin
SYSTEMD_SERVICE:${PN}:remove = "setup_i2c.service power-on.service"
