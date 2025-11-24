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

inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

LOCAL_URI += " \
    file://aconf_util.sh \
    file://bios_util.sh \
    file://bmc_aboot.conf \
    file://board-utils.sh \
    file://cpu_aboot.conf \
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
    file://yamp_flash.layout \
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
    aconf_util.sh \
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

do_work_systemd() {
    install -d ${D}/usr/local/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 setup_i2c.sh ${D}/usr/local/bin/setup_i2c.sh

    # networking is done after rcS, any start level within rcS
    # for mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}/usr/local/bin/eth0_mac_fixup.sh

    install -m 755 setup_board.sh ${D}/usr/local/bin/setup_board.sh

    install -m 755 power-on.sh ${D}/usr/local/bin/power-on.sh
}

do_work_sysv() {
    # the script to mount /mnt/data
    install -m 0755 ${UNPACKDIR}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 03 S .

    install -m 0755 ${UNPACKDIR}/rc.early ${D}${sysconfdir}/init.d/rc.early
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

    install -m 0755 ${UNPACKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0644 ${UNPACKDIR}/bmc_aboot.conf ${D}${sysconfdir}/bmc_aboot.conf
    install -m 0644 ${UNPACKDIR}/cpu_aboot.conf ${D}${sysconfdir}/cpu_aboot.conf
}

do_install:append() {
    # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
    olddir="/usr/local/fbpackages/utils"
    install -d ${D}${olddir}
    ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      do_work_systemd
    else
      do_work_sysv
    fi

    install -m 0755 ${UNPACKDIR}/yamp_flash.layout ${D}${sysconfdir}/yamp_flash.layout
}

FILES:${PN} += "${sysconfdir}"

SYSTEMD_SERVICE:${PN} += "setup_i2c.service"
