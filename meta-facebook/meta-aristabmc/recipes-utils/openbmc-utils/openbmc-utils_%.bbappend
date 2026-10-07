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
    file://aristabmc_cpu_flash.layout \
    file://bios_util.sh \
    file://bios_ver.sh \
    file://board-utils.sh \
    file://cpld_ver.sh \
    file://oob-mdio-util.sh \
    file://setup-gpio.sh \
    file://setup_i2c.sh \
    file://switchToCpu.sh \
    "

OPENBMC_UTILS_FILES += "\
    bios_util.sh \
    bios_ver.sh \
    cpld_ver.sh \
    oob-mdio-util.sh \
    switchToCpu.sh \
    "

do_install:append() {
    install -m 0644 ${UNPACKDIR}/aristabmc_cpu_flash.layout \
        ${D}${sysconfdir}/aristabmc_cpu_flash.layout
}

FILES:${PN} += "${sysconfdir}/aristabmc_cpu_flash.layout"
