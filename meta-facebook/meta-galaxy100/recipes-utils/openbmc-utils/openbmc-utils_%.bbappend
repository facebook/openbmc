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

inherit systemd

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://board-utils.sh \
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
      file://sensors_config_fix.service \
      file://fix_fru_eeprom.py \
      file://fix_fru_eeprom.service \
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
      "

RDEPENDS_${PN} += " python3 bash"

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

    install -m 755 setup_i2c.sh ${D}${localbindir}

    install -m 755 fix_fru_eeprom.py ${D}${localbindir}/fix_fru_eeprom.py
    install -m 644 fix_fru_eeprom.service ${D}${systemd_system_unitdir}

    install -m 644 us_monitor.service ${D}${systemd_system_unitdir}

    install -m 755 eth0_mac_fixup.sh ${D}${localbindir}/eth0_mac_fixup.sh

    install -m 755 power-on.sh ${D}${localbindir}/power-on.sh

    install -m 755 sensors_config_fix.sh ${D}${localbindir}
    install -m 644 sensors_config_fix.service ${D}${systemd_system_unitdir}

}

FILES_${PN} += "${sysconfdir}"

SYSTEMD_SERVICE_${PN} += "fix_fru_eeprom.service \
                      us_monitor.service \
                      sensors_config_fix.service \
"
