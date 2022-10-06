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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

LOCAL_URI += "\
    file://board-utils.sh \
    file://bios_util.sh \
    file://fpga_util.sh \
    file://fpga_ver.sh \
    file://dump_pim_serials.sh \
    file://dump_gpios.sh \
    file://eth0_mac_fixup.sh \
    file://oob-eeprom-util.sh \
    file://oob-mdio-util.sh \
    file://oob-status.sh \
    file://power-on.sh \
    file://setup_board.sh \
    file://wedge_power.sh \
    file://wedge_us_mac.sh \
    file://setup-gpio.sh \
    file://setup_i2c.sh \
    file://seutil \
    file://dpm_dump.sh \
    file://dpm_utils.sh \
    file://dpm_ver.sh \
    file://pim_dpm_dump.sh \
    file://show_tech.py \
    file://psu_show_tech.py \
    file://pim_types.sh \
    file://elbert_pim.layout \
    file://peutil \
    file://spi_pim_ver.sh \
    file://meta_info.sh \
    file://beacon_led.sh \
    file://setup_bcm53134.sh \
    file://th4_qspi_ver.sh \
    file://dpeCheck.sh \
    file://bmc_board_rev.sh \
    "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    bios_util.sh \
    fpga_util.sh \
    fpga_ver.sh \
    dump_pim_serials.sh \
    dump_gpios.sh \
    oob-eeprom-util.sh \
    oob-mdio-util.sh \
    oob-status.sh \
    wedge_power.sh \
    wedge_us_mac.sh \
    setup_i2c.sh \
    seutil \
    dpm_dump.sh \
    dpm_utils.sh \
    pim_dpm_dump.sh \
    dpm_ver.sh \
    show_tech.py \
    psu_show_tech.py \
    pim_types.sh \
    peutil \
    spi_pim_ver.sh \
    meta_info.sh \
    beacon_led.sh \
    th4_qspi_ver.sh \
    dpeCheck.sh \
    bmc_board_rev.sh \
    "

DEPENDS:append = " update-rc.d-native"

do_install_board() {
    # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
    olddir="/usr/local/fbpackages/utils"
    install -d ${D}${olddir}
    ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    # the script to mount /mnt/data
    install -m 0755 ${S}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 03 S .
    install -m 0755 ${S}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 04 S .

    install -m 755 dpm_dump.sh ${D}${sysconfdir}/init.d/dpm_dump.sh
    update-rc.d -r ${D} dpm_dump.sh start 50 S .

    # Export GPIO pins and set initial directions/values.
    install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
    update-rc.d -r ${D} setup-gpio.sh start 59 S .

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 60 S .

    # networking is done after rcS, any start level within rcS for
    # mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    # Fixes the speed on the BCM53134. This can be done in any start level
    # within rcS.
    install -m 755 setup_bcm53134.sh ${D}${sysconfdir}/init.d/setup_bcm53134.sh
    update-rc.d -r ${D} setup_bcm53134.sh start 75 S .

    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 80 S .

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
    update-rc.d -r ${D} power-on.sh start 85 S .

    install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${S}/elbert_pim.layout ${D}${sysconfdir}/elbert_pim.layout
}

do_install:append() {
  do_install_board
}

FILES:${PN} += "${sysconfdir}"
