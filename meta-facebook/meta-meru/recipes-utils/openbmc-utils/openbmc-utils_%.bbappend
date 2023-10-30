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
    file://aconf_util.sh \
    file://bios-util.sh \
    file://bmc_aboot.conf \
    file://board-utils.sh \
    file://cpu_aboot.conf \
    file://fpga-util.sh \
    file://oob-mdio-util.sh \
    file://meru_flash.layout \
    file://meru_flash_32m.layout \
    file://setup_i2c.sh \
    file://setup-gpio.sh \
    file://setup_board.sh \
    "

OPENBMC_UTILS_FILES += " \
    aconf_util.sh \
    bios_util.sh \
    fpga_util.sh \
    oob-mdio-util.sh \
    "

do_install_bios_layout() {
    install -m 0644 ${S}/bmc_aboot.conf ${D}${sysconfdir}/bmc_aboot.conf
    install -m 0644 ${S}/cpu_aboot.conf ${D}${sysconfdir}/cpu_aboot.conf
    install -m 0644 ${S}/meru_flash.layout ${D}${sysconfdir}/meru_flash.layout
    install -m 0644 ${S}/meru_flash_32m.layout ${D}${sysconfdir}/meru_flash_32m.layout
}

do_install:append() {
    do_install_bios_layout
}

FILES:${PN} += "${sysconfdir}"
