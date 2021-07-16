# Copyright 2020-present Delta Eletronics, Inc. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files/delta:"

SRC_URI += "file://board-utils.sh \
            file://boot_info.sh \
            file://cpld_ver.sh \
            file://disable_watchdog.sh \
            file://eth0_mac_fixup.sh \
            file://feutil \
            file://presence_util.sh \
            file://reset_brcm.sh \
            file://set_vdd.sh \
            file://setup_avs.sh \
            file://setup_board.sh \
            file://setup_default_gpio.sh \
            file://setup_i2c.sh \
            file://setup_mgmt.sh \
            file://smbcpld_update.sh \
            file://spi_util.sh \
            file://switch_reset.sh \
            file://us_console.sh \
           "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    boot_info.sh \
    cpld_ver.sh \
    disable_watchdog.sh \
    feutil \
    presence_util.sh \
    reset_brcm.sh \
    set_vdd.sh \
    setup_avs.sh \
    setup_mgmt.sh \
    smbcpld_update.sh \
    spi_util.sh \
    switch_reset.sh \
    us_console.sh \
    "

DEPENDS_append = " update-rc.d-native"

do_install_board() {
    # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
    olddir="/usr/local/fbpackages/utils"
    install -d ${D}${olddir}
    ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    # the script to mount /mnt/data
    install -m 0755 ${WORKDIR}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 03 S .
    install -m 0755 ${WORKDIR}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 04 S .

    # install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
    # update-rc.d -r ${D} power-on.sh start 85 S .

    install -m 755 setup_default_gpio.sh ${D}${sysconfdir}/init.d/setup_default_gpio.sh
    update-rc.d -r ${D} setup_default_gpio.sh start 60 S .

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 59 S .

    install -m 755 setup_avs.sh ${D}${sysconfdir}/init.d/setup_avs.sh
    update-rc.d -r ${D} setup_avs.sh start 65 S .

    # networking is done after rcS, any start level within rcS
    # for mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 80 S .

    install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/disable_watchdog.sh ${D}${sysconfdir}/init.d/disable_watchdog.sh
    update-rc.d -r ${D} disable_watchdog.sh start 99 2 3 4 5 .

}

do_install_append() {
  do_install_board
}

FILES_${PN} += "${sysconfdir}"
