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

SRC_URI += " \
    file://board-utils.sh \
    file://cmmcpld_update.sh \
    file://create_vlan_intf \
    file://eth0_mac_fixup.sh \
    file://fcbcpld_update.sh \
    file://fix_fru_eeprom.py \
    file://get_fan_speed.sh \
    file://presence_util.sh \
    file://setup_adc.sh \
    file://setup_gpio.sh \
    file://setup_i2c.sh \
    file://set_fan_speed.sh \
    file://sol.sh \
    file://ceutil \
    file://system_monitor.sh \
    file://setup_system_monitor.sh \
    file://cmm-modules-devices-reset.sh \
    "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    cmmcpld_update.sh \
    fcbcpld_update.sh \
    get_fan_speed.sh \
    presence_util.sh \
    set_fan_speed.sh \
    sol.sh \
    ceutil \
    system_monitor.sh \
    setup_system_monitor.sh \
    cmm-modules-devices-reset.sh \
    "

DEPENDS_append = " update-rc.d-native"

RDEPENDS_${PN} += "python3-core"

do_install_append() {
    # the script to mount emmc to /var/log
    install -m 0755 ${WORKDIR}/setup_persist_log.sh ${D}${sysconfdir}/init.d/setup_persist_log.sh
    update-rc.d -r ${D} setup_persist_log.sh start 05 S .

    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 80  S .

    # EEPROM is loaded in setup_i2c.sh
    install -m 755 fix_fru_eeprom.py ${D}${sysconfdir}/init.d/fix_fru_eeprom.py
    update-rc.d -r ${D} fix_fru_eeprom.py start 81 S .

    # networking is started after rcS, any start level within rcS after loading
    # EEPROM should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 99 S .

    install -m 755 setup_adc.sh ${D}${sysconfdir}/init.d/setup_adc.sh
    update-rc.d -r ${D} setup_adc.sh start 80 2 3 5 .

    install -m 755 setup_gpio.sh ${D}${sysconfdir}/init.d/setup_gpio.sh
    update-rc.d -r ${D} setup_gpio.sh start 80 2 3 5 .

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    install -m 755 setup_system_monitor.sh ${D}${sysconfdir}/init.d/setup_system_monitor.sh
    update-rc.d -r ${D} setup_system_monitor.sh start 98 2 3 5 .
}
