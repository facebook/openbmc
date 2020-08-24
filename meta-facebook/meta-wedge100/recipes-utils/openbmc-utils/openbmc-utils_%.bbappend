# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the License.
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
            file://internal_switch_version.sh \
            file://cp2112_i2c_flush.sh \
            file://reset_qsfp_mux.sh \
            file://setup_i2c.sh \
            file://setup_i2c.service \
            file://power-on.service \
            file://setup_board.service \
            file://mount_data0.sh \
            file://setup_sensors_conf.sh \
            file://spi_util.sh \
           "

OPENBMC_UTILS_FILES += " \
    disable_watchdog.sh \
    enable_watchdog_ext_signal.sh \
    cpld_upgrade.sh \
    cpld_rev.sh \
    internal_switch_version.sh \
    ec_version.sh \
    cp2112_i2c_flush.sh \
    reset_qsfp_mux.sh \
    spi_util.sh \
    "

DEPENDS_append = " update-rc.d-native"

inherit systemd

do_install_board() {
    # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
    olddir="/usr/local/fbpackages/utils"
    install -d ${D}${olddir}
    ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}/usr/local/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 setup_i2c.sh ${D}/usr/local/bin

    install -m 0755 mount_data0.sh ${D}/usr/local/bin

    # networking is done after rcS, any start level within rcS
    # for mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}/usr/local/bin/eth0_mac_fixup.sh

    install -m 755 setup_board.sh ${D}/usr/local/bin/setup_board.sh

    install -m 755 power-on.sh ${D}/usr/local/bin/power-on.sh

    install -m 0755 ${WORKDIR}/disable_watchdog.sh ${D}/usr/local/bin/disable_watchdog.sh

    install -m 0755 ${WORKDIR}/enable_watchdog_ext_signal.sh ${D}/usr/local/bin/enable_watchdog_ext_signal.sh

}

FILES_${PN} += "${sysconfdir}"
