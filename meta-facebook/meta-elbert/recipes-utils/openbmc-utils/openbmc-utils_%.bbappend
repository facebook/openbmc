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

PACKAGECONFIG += "disable-watchdog"

SRC_URI += "file://board-utils.sh \
            file://boot_info.sh \
            file://bios_util.sh \
            file://fpga_util.sh \
            file://fpga_ver.sh \
            file://dump_pim_serials.sh \
            file://dump_gpios.sh \
            file://eth0_mac_fixup.sh \
            file://hclk_fixup.sh \
            file://oob-eeprom-util.sh \
            file://oob-mdio-util.sh \
            file://power-on.sh \
            file://setup_board.sh \
            file://wedge_power.sh \
            file://wedge_us_mac.sh \
            file://setup-gpio.sh \
            file://setup_i2c.sh \
            file://seutil \
            file://dpm_ver.sh \
            file://show_tech.py \
            file://psu_show_tech.py \
            file://pim_enable.sh \
            file://pim_types.sh \
            file://elbert_pim.layout \
            file://peutil \
            file://spi_pim_ver.sh \
            file://meta_info.sh \
            file://beacon_led.sh \
           "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    bios_util.sh \
    boot_info.sh \
    fpga_util.sh \
    fpga_ver.sh \
    dump_pim_serials.sh \
    dump_gpios.sh \
    oob-eeprom-util.sh \
    oob-mdio-util.sh \
    wedge_power.sh \
    wedge_us_mac.sh \
    setup_i2c.sh \
    seutil \
    dpm_ver.sh \
    show_tech.py \
    psu_show_tech.py \
    pim_enable.sh \
    pim_types.sh \
    peutil \
    spi_pim_ver.sh \
    meta_info.sh \
    beacon_led.sh \
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

    # Export GPIO pins and set initial directions/values.
    install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
    update-rc.d -r ${D} setup-gpio.sh start 59 S .

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 60 S .

    # ELBERTTODO Remove P1 Hacks
    install -m 755 hclk_fixup.sh ${D}${sysconfdir}/init.d/hclk_fixup.sh
    update-rc.d -r ${D} hclk_fixup.sh start 70 S .

    # networking is done after rcS, any start level within rcS for
    # mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 80 S .

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
    update-rc.d -r ${D} power-on.sh start 85 S .

    install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/elbert_pim.layout ${D}${sysconfdir}/elbert_pim.layout
}

do_install_append() {
  do_install_board
}

FILES_${PN} += "${sysconfdir}"
