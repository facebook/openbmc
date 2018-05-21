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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://disable_watchdog.sh \
            file://enable_watchdog_ext_signal.sh \
            file://ec_version.sh \
            file://board-utils.sh \
            file://setup_board.sh \
            file://cpld_rev.sh \
            file://cpld_upgrade.sh \
            file://cp2112_i2c_flush.sh \
            file://reset_qsfp_mux.sh \
            file://setup_i2c.sh \
            file://btools.py \
            file://rest_mntr.sh \
            file://mav_tty_switch_delay.sh \
           "

OPENBMC_UTILS_FILES += " \
    disable_watchdog.sh \
    enable_watchdog_ext_signal.sh \
    cpld_upgrade.sh \
    cpld_rev.sh \
    ec_version.sh \
    cp2112_i2c_flush.sh \
    reset_qsfp_mux.sh \
    mav_tty_switch_delay.sh \
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

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 60 S .

    # networking is done after rcS, any start level within rcS
    # for mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 80 S .

    install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
    update-rc.d -r ${D} power-on.sh start 85 S .

    install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/disable_watchdog.sh ${D}${sysconfdir}/init.d/disable_watchdog.sh
    update-rc.d -r ${D} disable_watchdog.sh start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/enable_watchdog_ext_signal.sh ${D}${sysconfdir}/init.d/enable_watchdog_ext_signal.sh
    update-rc.d -r ${D} enable_watchdog_ext_signal.sh start 99 2 3 4 5 .

    install -d ${D}/usr/local/fbpackages/rest-api/
    install -m 0755 ${WORKDIR}/btools.py ${D}/usr/local/fbpackages/rest-api/btools.py
    ln -snf "/usr/local/fbpackages/rest-api/btools.py" ${D}/usr/local/bin/btools.py
    install -m 0755 ${WORKDIR}/rest_mntr.sh ${D}/usr/local/bin/rest_mntr.sh
}

FILES_${PN} += "${sysconfdir}"
