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
    file://power-on.sh \
    file://setup-gpio.sh \
    file://setup_board.sh \
    file://setup_i2c.sh \
    file://wedge_power.sh \
    file://wedge_us_mac.sh \
    file://eth0_mac_fixup.sh \
    "

OPENBMC_UTILS_FILES += " \
    board-utils.sh \
    power-on.sh \
    setup-gpio.sh \
    setup_board.sh \
    setup_i2c.sh \
    wedge_power.sh \
    wedge_us_mac.sh \
    eth0_mac_fixup.sh \
    "

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

DEPENDS:append = " update-rc.d-native"

do_work_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    # the script to mount /mnt/data
    install -m 0755 ${S}/mount_data0.sh ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} mount_data0.sh start 03 S .

    install -m 0755 ${S}/rc.early ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} rc.early start 04 S .

    # Launch board specific configurations
    install -m 0755 setup_board.sh ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} setup_board.sh start 10 S .

    # Export GPIO pins and set initial directions/values.
    install -m 0755 setup-gpio.sh ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} setup-gpio.sh start 59 S .

    install -m 0755 setup-gpio.sh ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} setup_i2c.sh start 59 S .

    # networking is done after rcS, any start level within rcS for
    # mac fixup should work
    install -m 0755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 0755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/

    install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .
}

do_work_systemd() {
    install -d ${D}/usr/local/bin
    install -d ${D}${systemd_system_unitdir}

    for svc in mount_data1.service setup_board.service \
               setup_gpio.service setup_i2c.service; do
        install -m 0644 ${svc} ${D}${systemd_system_unitdir}
    done
}

do_install:append() {
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      do_work_systemd
    else
      do_work_sysv
    fi
}

FILES:${PN} += "${sysconfdir}"

SYSTEMD_SERVICE:${PN} += "mount_data1.service"
SYSTEMD_SERVICE:${PN} += "setup_board.service"
SYSTEMD_SERVICE:${PN} += "setup_gpio.service"
SYSTEMD_SERVICE:${PN} += "setup_i2c.service"

# Not needed for fboss-lite openbmc systems.
SYSTEMD_SERVICE:${PN}:remove = "enable_watchdog_ext_signal.service"
