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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://board-utils.sh \
    file://power-on.sh \
    file://wedge_power.sh \
    file://us_monitor.sh \
    file://us_refresh.sh \
    file://us_console.sh \
    file://bios_flash.sh \
    file://bcm5389.sh \
    file://at93cx6_util.sh \
    file://galaxy100_bmc.py \
    file://lsb_release \
    file://seutil \
    file://sensors_config_fix.sh \
    file://fix_fru_eeprom.py \
    file://qsfp_cpld_ver.sh \
    file://ceutil.py \
    file://version_dump \
    file://cpldupgrade \
    file://repeater_verify.sh \
    file://setup_i2c.sh \
    file://scm_cpld_rev.sh \
    file://ec_version.sh \
    file://create_vlan_intf \
    file://us_monitor.service \
    file://fix_fru_eeprom.service \
    file://sensors_config_fix.service \
    "

RDEPENDS:${PN} += " python3 bash"
DEPENDS:append = " update-rc.d-native"

inherit systemd

install_board_sysv() {
    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    # rc.S

    # the script to mount /mnt/data
    install -m 0755 ${S}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 03 S .

    install -m 0755 ${S}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 04 S .

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 60 S .

    install -m 755 fix_fru_eeprom.py ${D}${sysconfdir}/init.d/fix_fru_eeprom.py
    update-rc.d -r ${D} fix_fru_eeprom.py start 61 S .

    # networking is done after rcS, any start level within rcS
    # for mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    install -m 755 start_us_monitor.sh ${D}${sysconfdir}/init.d/start_us_monitor.sh
    update-rc.d -r ${D} start_us_monitor.sh start 84 S .

    install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
    update-rc.d -r ${D} power-on.sh start 85 S .

    # rc.[2345]

    install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 755 sensors_config_fix.sh ${D}${sysconfdir}/init.d/sensors_config_fix.sh
    update-rc.d -r ${D} sensors_config_fix.sh start 100 2 3 4 5 .
}

install_board_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 us_monitor.service ${D}${systemd_system_unitdir}
    install -m 644 fix_fru_eeprom.service ${D}${systemd_system_unitdir}
    install -m 644 sensors_config_fix.service ${D}${systemd_system_unitdir}
    install -m 755 eth0_mac_fixup.sh ${D}${localbindir}
    install -m 755 setup_i2c.sh ${D}${localbindir}
    # This one comes from the wedge layer but we don't want it on galaxy
    systemctl --root=${D} mask setup_board.service
}

do_install_board() {
    # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
    olddir="/usr/local/fbpackages/utils"
    localbindir="/usr/local/bin"
    bindir="/usr/bin"
    install -d ${D}${olddir}
    install -d ${D}${localbindir}
    install -d ${D}${bindir}
    ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

    install -m 0755 bios_flash.sh ${D}${localbindir}/bios_flash.sh
    install -m 0755 bcm5389.sh ${D}${localbindir}/bcm5389.sh
    install -m 0755 at93cx6_util.sh ${D}${localbindir}/at93cx6_util.sh
    install -m 0755 lsb_release ${D}${localbindir}/lsb_release
    install -m 0755 cpldupgrade ${D}${localbindir}/cpldupgrade
    install -m 0755 us_refresh.sh ${D}${localbindir}/us_refresh.sh
    install -m 0755 repeater_verify.sh ${D}${localbindir}/repeater_verify.sh
    install -m 0755 seutil ${D}${bindir}/seutil
    install -m 0755 version_dump ${D}${bindir}/version_dump
    install -m 0755 qsfp_cpld_ver.sh ${D}${localbindir}/qsfp_cpld_ver.sh
    install -m 0755 ceutil.py ${D}${localbindir}/ceutil
    install -m 0755 scm_cpld_rev.sh ${D}${localbindir}/scm_cpld_rev.sh
    install -m 0755 ec_version.sh ${D}${localbindir}/ec_version.sh

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_board_systemd
    else
        install_board_sysv
    fi
}

FILES:${PN} += "${sysconfdir}"

SYSTEMD_SERVICES:${PN} += "us_monitor.service \
                       fix_fru_eeprom.service \
                        sensors_config_fix.service \
                        setup_i2c.service"

SYSTEMD_SERVICES:${PN}:remove = "setup_board.service"
