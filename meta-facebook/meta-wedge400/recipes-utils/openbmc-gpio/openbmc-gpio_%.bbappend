# Copyright 2019-present Facebook. All Rights Reserved.
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

inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
          file://board_gpio_table_v1.py \
          file://board_gpio_rev_table.py \
          file://openbmc_gpio_setup.py \
          file://setup_board.py \
          file://openbmc_gpio_setup.service \
          "

OPENBMC_GPIO_SOC_TABLE = "ast2500_gpio_table.py"
DEPENDS += "update-rc.d-native"

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 openbmc_gpio_setup.py ${D}${sysconfdir}/init.d/openbmc_gpio_setup.py
    update-rc.d -r ${D} openbmc_gpio_setup.py start 58 S .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 755 openbmc_gpio_setup.py ${D}/usr/local/bin/openbmc_gpio_setup.py
    install -m 0644 openbmc_gpio_setup.service ${D}${systemd_system_unitdir}
}

do_install:append() {
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi
}

FILES:${PN} += "/usr/local/bin ${sysconfdir}"
SYSTEMD_SERVICE:${PN} = "openbmc_gpio_setup.service"
