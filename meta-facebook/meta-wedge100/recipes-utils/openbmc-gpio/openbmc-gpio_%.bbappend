# Copyright 2014-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

inherit systemd

SRC_URI += " \
          file://openbmc-gpio-1/board_gpio_table_v1.py \
          file://openbmc-gpio-1/board_gpio_table_v2.py \
          file://openbmc-gpio-1/board_gpio_rev_table.py \
          file://openbmc-gpio-1/openbmc_gpio_setup.py \
          file://openbmc-gpio-1/setup_board.py \
          file://openbmc-gpio-1/openbmc_gpio_setup.service \
"

OPENBMC_GPIO_SOC_TABLE = "ast2400_gpio_table.py"

do_install_append() {
     install -d ${D}${sysconfdir}/init.d
     install -d ${D}${sysconfdir}/rcS.d
     install -d ${D}${systemd_system_unitdir}
     install -m 755 openbmc_gpio_setup.py ${D}/usr/local/bin/openbmc_gpio_setup.py
     install -m 0644 openbmc_gpio_setup.service ${D}${systemd_system_unitdir}
}

FILES_${PN} += "/usr/local/bin ${sysconfdir}"

SYSTEMD_SERVICE_${PN} = "openbmc_gpio_setup.service"
