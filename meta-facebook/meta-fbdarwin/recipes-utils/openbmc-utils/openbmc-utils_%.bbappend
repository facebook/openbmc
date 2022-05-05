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

LOCAL_URI += "\
    file://board-utils.sh \
    file://dump_gpios.sh \
    file://eth0_mac_fixup.sh \
    file://find_serfmon.sh \
    file://meta_info.sh \
    file://oob-eeprom-util.sh \
    file://oob-mdio-util.sh \
    file://oob-status.sh \
    file://setup-gpio.sh \
    file://setup_bcm53134.sh \
    file://setup_board.sh \
    file://show_tech.py \
    file://wedge_power.sh \
    "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    dump_gpios.sh \
    find_serfmon.sh \
    meta_info.sh \
    oob-eeprom-util.sh \
    oob-mdio-util.sh \
    oob-status.sh \
    show_tech.py \
    wedge_power.sh \
    "

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

DEPENDS:append = " update-rc.d-native"

do_install_board() {
    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    # the script to mount /mnt/data
    install -m 0755 ${S}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 03 S .

    install -m 0755 ${S}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 04 S .

    # Install find_serfmon to print it early in case of cached cases
    install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
    update-rc.d -r ${D} setup_board.sh start 10 S .

    # Export GPIO pins and set initial directions/values.
    install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
    update-rc.d -r ${D} setup-gpio.sh start 59 S .

    # networking is done after rcS, any start level within rcS for
    # mac fixup should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .
}

do_install:append() {
  do_install_board
}

FILES:${PN} += "${sysconfdir}"
