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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

LOCAL_URI += " \
    file://bios_util.sh \
    file://board-utils.sh \
    file://eth0_mac_fixup.sh \
    file://fpga_util.sh \
    file://fpga_ver.sh \
    file://power-on.sh \
    file://reset_brcm.sh \
    file://setup_board.sh \
    file://setup_i2c.sh \
    file://wedge_power.sh \
    file://wedge_us_mac.sh \
    file://pim_enable.sh \
    file://dpm_ver.sh \
    file://dump_pim_serials.sh \
    file://yamp_bios.layout \
    file://showtech.sh \
    file://seutil \
    file://peutil \
    file://scdinfo \
    file://psu_show_tech.py \
    file://show_tech.py \
    file://sup_eeprom.sh \
    file://dpm_dump.sh \
    "

OPENBMC_UTILS_FILES += " \
    bios_util.sh \
    board-utils.sh \
    fpga_util.sh \
    fpga_ver.sh \
    reset_brcm.sh \
    wedge_power.sh \
    wedge_us_mac.sh \
    dpm_ver.sh \
    dump_pim_serials.sh \
    pim_enable.sh \
    showtech.sh \
    seutil \
    peutil \
    scdinfo \
    psu_show_tech.py \
    show_tech.py \
    dpm_dump.sh \
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

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 60 S .

    # "eth0_mac_fixup.sh" needs to be executed after "networking start"
    # (runlevel 5, order #1), but before "setup-dhc6.sh" (runlevel 5,
    # order #3): this is to make sure ipv6 link-local address can be
    # derivied from the correct MAC address.
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 2 2 3 4 5 .

    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 80 S .

    install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
    update-rc.d -r ${D} power-on.sh start 85 S .

    install -m 755 sup_eeprom.sh ${D}${sysconfdir}/init.d/sup_eeprom.sh
    update-rc.d -r ${D} sup_eeprom.sh start 90 2 3 4 5 .

    install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${S}/yamp_bios.layout ${D}${sysconfdir}/yamp_bios.layout
}

do_install:append() {
  do_install_board
}

FILES:${PN} += "${sysconfdir}"
