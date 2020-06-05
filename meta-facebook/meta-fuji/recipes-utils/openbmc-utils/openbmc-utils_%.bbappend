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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://setup-gpio.sh \
            file://board-utils.sh \
            file://setup_board.sh \
            file://setup_i2c.sh \
            file://sol.sh \
            file://cpld_update.sh \
            file://wedge_power.sh \
           "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    sol.sh \
    cpld_update.sh \
    wedge_power.sh \
    "
DEPENDS_append = " update-rc.d-native"

do_install_board() {
    # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
    olddir="/usr/local/fbpackages/utils"
    install -d ${D}${olddir}
    ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"
    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 59 S .
    # Export GPIO pins and set initial directions/values.
    install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
    update-rc.d -r ${D} setup-gpio.sh start 59 S .

    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 80 S .

}

do_install_append() {
  do_install_board
}

FILES_${PN} += "${sysconfdir}"
