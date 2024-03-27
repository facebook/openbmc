# Copyright (c) Meta Platforms, Inc. and affiliates.
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
    file://board-utils.sh \
    file://setup_i2c.sh \
    file://setup-gpio.sh\
    file://spi_util.sh \
    file://read_INA230.sh \
    file://cpld_update.sh \
    file://setup_board.sh \
    "

OPENBMC_UTILS_FILES += " \
    spi_util.sh \
    read_INA230.sh \
    cpld_update.sh \
    "
