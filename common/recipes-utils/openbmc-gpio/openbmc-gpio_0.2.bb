# Copyright 2015-present Facebook. All Rights Reserved.
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

SUMMARY = "OpenBMC GPIO utilies"
SECTION = "base"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

SRC_URI = " \
    file://openbmc-gpio-1/COPYING \
    file://openbmc-gpio-1/board_gpio_table.py \
    file://openbmc-gpio-1/board_passthrough_gpio_table.py \
    file://openbmc-gpio-1/board_tolerance_gpio_table.py \
    file://openbmc-gpio-1/openbmc_gpio.py \
    file://openbmc-gpio-1/openbmc_gpio_table.py \
    file://openbmc-gpio-1/openbmc_gpio_util.py \
    file://openbmc-gpio-1/phymemory.py \
    file://openbmc-gpio-1/setup.py \
    file://openbmc-gpio-1/soc_gpio.py \
    file://openbmc-gpio-1/soc_gpio_table.py \
    file://openbmc-gpio-1/setup_board.py \
    "

S = "${WORKDIR}/openbmc-gpio-1"

OPENBMC_GPIO_UTILS = " \
    openbmc_gpio_util.py \
    "

OPENBMC_GPIO_SOC_TABLE = "soc_gpio_table.py"

inherit distutils3

DEPENDS_${PN} = "python3 python3-distribute update-rc.d-native"

RDEPENDS_${PN} = "python3-core python3-argparse python3-subprocess"

do_board_defined_soc_table() {
    if [ "${OPENBMC_GPIO_SOC_TABLE}" != "soc_gpio_table.py" ]; then
        mv -f "${S}/${OPENBMC_GPIO_SOC_TABLE}" "${S}/soc_gpio_table.py"
    fi
}
addtask board_defined_soc_table after do_unpack before do_build

do_install_append() {
    localbindir="${D}/usr/local/bin"
    install -d ${localbindir}
    for f in ${OPENBMC_GPIO_UTILS}; do
        install -m 755 $f ${localbindir}/${f}
    done
}

