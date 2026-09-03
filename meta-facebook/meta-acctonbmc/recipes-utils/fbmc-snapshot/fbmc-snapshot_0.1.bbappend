# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
    file://900_aspeed_chip_rev.sh \
    file://901_boot_info.sh \
    file://902_oob_status.sh \
    file://903_network_status.sh \
    file://904_i2c_detect.sh \
    file://905_sensors.sh \
    file://906_dump_gpio.sh \
    file://907_boot_console_log.sh \
    file://908_mterm_rotated.sh \
    file://aspeed_chip_rev.sh \
    file://bmc_board_rev.sh \
    file://dump_gpios.sh \
    file://meta_info.sh \
    file://oob-mdio-util.sh \
    file://oob-status.sh \
    "

# showtech plugins, executed in name order out of /etc/showtech/rules/
SHOWTECH_RULES_FILES:append = " \
    900_aspeed_chip_rev.sh \
    901_boot_info.sh \
    902_oob_status.sh \
    903_network_status.sh \
    904_i2c_detect.sh \
    905_sensors.sh \
    906_dump_gpio.sh \
    907_boot_console_log.sh \
    908_mterm_rotated.sh \
    "

# Standalone debug utilities. These used to be shipped by the show-tech
# recipe; they are invoked by the rules above and are also used interactively,
# so they stay in /usr/local/bin.
SHOWTECH_UTILS_FILES = " \
    aspeed_chip_rev.sh \
    bmc_board_rev.sh \
    dump_gpios.sh \
    meta_info.sh \
    oob-mdio-util.sh \
    oob-status.sh \
    "

do_install:append() {
    localbindir="${D}/usr/local/bin"
    install -d ${localbindir}

    for f in ${SHOWTECH_UTILS_FILES}; do
        install -m 755 ${S}/$f ${localbindir}/${f}
    done
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "/usr/local/bin"
