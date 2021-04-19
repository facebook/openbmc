# Copyright 2018-present Facebook. All Rights Reserved.
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

PACKAGECONFIG += "disable-watchdog"

SRC_URI += "file://board-utils.sh \
            file://boot_info.sh \
            file://cpld_ver.sh \
            file://fcmcpld_update.sh \
            file://fpga_ver.sh \
            file://feutil \
            file://pdbcpld_update.sh \
            file://peutil \
            file://pimcpld_update.sh \
            file://power-on.sh \
            file://presence_util.sh \
            file://reset_brcm.sh \
            file://scmcpld_update.sh \
            file://set_pim_sensor.sh \
            file://dump_pim_serials.sh \
            file://set_sled.sh \
            file://setup_board.sh \
            file://setup_i2c.sh \
            file://setup_mgmt.sh \
            file://setup_pcie_repeater.sh \
            file://setup_qsfp.sh \
            file://setup_sensors_conf.sh \
            file://seutil \
            file://smbcpld_update.sh \
            file://sol.sh \
            file://spi_util.sh \
            file://us_console.sh \
            file://read_sled.sh \
            file://reset_cp2112.sh \
            file://wedge_power.sh \
            file://wedge_us_mac.sh \
            file://create_vlan_intf \
           "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    boot_info.sh \
    cpld_ver.sh \
    fcmcpld_update.sh \
    fpga_ver.sh \
    feutil \
    presence_util.sh \
    peutil \
    pdbcpld_update.sh \
    pimcpld_update.sh \
    reset_brcm.sh \
    scmcpld_update.sh \
    set_pim_sensor.sh \
    dump_pim_serials.sh \
    set_sled.sh \
    setup_mgmt.sh \
    setup_pcie_repeater.sh \
    setup_qsfp.sh \
    seutil \
    smbcpld_update.sh \
    sol.sh \
    spi_util.sh \
    us_console.sh \
    read_sled.sh \
    reset_cp2112.sh \
    wedge_power.sh \
    wedge_us_mac.sh \
    "

DEPENDS_append = " update-rc.d-native"

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

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 60 S .

    # the script to setup lm-sensors conf
    install -m 755 setup_sensors_conf.sh ${D}${sysconfdir}/init.d/setup_sensors_conf.sh
    update-rc.d -r ${D} setup_sensors_conf.sh start 61 S .

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

}

do_install_append() {
  do_install_board
}

FILES_${PN} += "${sysconfdir}"
