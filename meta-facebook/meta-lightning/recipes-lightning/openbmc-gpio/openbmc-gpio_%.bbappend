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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
CFLAGS += " -Wall -Werror "

DEPENDS:append = "update-rc.d-native"

SRC_URI += " \
          file://openbmc-gpio-1/board_gpio_table.py \
          file://openbmc-gpio-1/board_passthrough_gpio_table.py \
          file://openbmc-gpio-1/board_tolerance_gpio_table.py \
          file://openbmc-gpio-1/openbmc_gpio_setup.py \
          file://openbmc-gpio-1/setup_board.py \
          "
OPENBMC_GPIO_SOC_TABLE = "ast2400_gpio_table.py"

FILES:${PN} += "/usr/local/bin ${sysconfdir}"

do_install:append() {
     install -d ${D}${sysconfdir}/init.d
     install -d ${D}${sysconfdir}/rcS.d
     install -m 755 openbmc_gpio_setup.py ${D}${sysconfdir}/init.d/openbmc_gpio_setup.py
     update-rc.d -r ${D} openbmc_gpio_setup.py start 59 5 .
}

FILES:${PN} += "/usr/local/bin ${sysconfdir}"
