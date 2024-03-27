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
    file://bios_util.sh \
    file://bmc_aboot.conf \
    file://bmc_board_rev.sh \
    file://board-utils.sh \
    file://cpu_aboot.conf \
    file://fpga_util.sh \
    file://fpga_ver.sh \
    file://meru_flash.layout \
    file://oob-mdio-util.sh \
    file://setup-gpio.sh \
    file://setup_board.sh \
    file://setup_i2c.sh \
    file://switchToCpu.sh \
    file://meru-21-eth0.4092.netdev \
    file://meru-21-eth0.4092.network \
    "

OPENBMC_UTILS_FILES += " \
    aconf_util.sh \
    bios_util.sh \
    bmc_board_rev.sh \
    fpga_util.sh \
    fpga_ver.sh \
    oob-mdio-util.sh \
    switchToCpu.sh \
    "
do_install_bios_layout() {
    install -m 0644 ${S}/bmc_aboot.conf ${D}${sysconfdir}/bmc_aboot.conf
    install -m 0644 ${S}/cpu_aboot.conf ${D}${sysconfdir}/cpu_aboot.conf
    install -m 0644 ${S}/meru_flash.layout ${D}${sysconfdir}/meru_flash.layout
}

do_install_vlan4092() {
    install -m 0644 ${S}/meru-21-eth0.4092.netdev ${D}${sysconfdir}
    install -m 0644 ${S}/meru-21-eth0.4092.network ${D}${sysconfdir}
}

do_install:append() {
    do_install_bios_layout
    do_install_vlan4092
}

FILES:${PN} += "${sysconfdir}"
