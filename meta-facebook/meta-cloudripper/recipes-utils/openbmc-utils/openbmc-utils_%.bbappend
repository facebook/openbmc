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

LOCAL_URI += " \
    file://beutil \
    file://board-utils.sh \
    file://cpld_update.sh \
    file://cpld_ver.sh \
    file://feutil \
    file://fpga_ver.sh \
    file://power-on.sh \
    file://presence_util.sh \
    file://read_sled.sh \
    file://set_sled.sh \
    file://set_vdd.sh \
    file://setup_bic.sh \
    file://setup_board.sh \
    file://setup_i2c.sh \
    file://setup_mgmt.sh \
    file://setup-gpio.sh \
    file://seutil \
    file://sol.sh \
    file://spi_util.sh \
    file://switch_reset.sh \
    file://wedge_power.sh \
    file://wedge_us_mac.sh \
    file://xdpe12284-hack.sh \
    file://mount_data1.service \
    file://setup_gpio.service \
    "

OPENBMC_UTILS_FILES += " \
    beutil \
    board-utils.sh \
    cpld_update.sh \
    cpld_ver.sh \
    feutil \
    fpga_ver.sh \
    presence_util.sh \
    read_sled.sh \
    set_sled.sh \
    set_vdd.sh \
    setup_bic.sh \
    setup_mgmt.sh \
    seutil \
    sol.sh \
    spi_util.sh \
    switch_reset.sh \
    wedge_power.sh \
    wedge_us_mac.sh \
    xdpe12284-hack.sh \
    "

DEPENDS:append = " update-rc.d-native"
inherit systemd

do_work_sysv() {
  # the script to mount /mnt/data
  install -m 0755 ${S}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
  update-rc.d -r ${D} mount_data0.sh start 03 S .

  install -m 0755 ${S}/rc.early ${D}${sysconfdir}/init.d/rc.early
  update-rc.d -r ${D} rc.early start 04 S .

  # mount secondary storage (emmc) to /mnt/data1
  install -m 0755 ${S}/mount_data1.sh ${D}${sysconfdir}/init.d/mount_data1.sh
  update-rc.d -r ${D} mount_data1.sh start 05 S .

  install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
  update-rc.d -r ${D} power-on.sh start 85 S .

  install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 60 S .

  install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
  update-rc.d -r ${D} setup_i2c.sh start 59 S .

  # networking is done after rcS, any start level within rcS
  # for mac fixup should work
  install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
  update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .

  install -m 755 setup_board.sh ${D}${sysconfdir}/init.d/setup_board.sh
  update-rc.d -r ${D} setup_board.sh start 80 S .

  # create VLAN intf automatically
  install -d ${D}/${sysconfdir}/network/if-up.d
  install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

  install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/rc.local
  update-rc.d -r ${D} rc.local start 99 2 3 4 5 .
}

do_work_systemd() {
  # TODO: We'd want to run all the logic here that is run in mound_data0.sh
  install -d ${D}/usr/local/bin
  install -d ${D}${systemd_system_unitdir}

  # mount secondary storage (emmc) to /mnt/data1
  install -m 755 ${S}/mount_data1.sh ${D}/usr/local/bin/mount_data1.sh

  install -m 755 setup-gpio.sh ${D}/usr/local/bin/setup-gpio.sh

  install -m 755 setup_i2c.sh ${D}/usr/local/bin/setup_i2c.sh

  # networking is done after rcS, any start level within rcS
  # for mac fixup should work
  install -m 755 eth0_mac_fixup.sh ${D}/usr/local/bin/eth0_mac_fixup.sh

  install -m 755 setup_board.sh ${D}/usr/local/bin/setup_board.sh

  install -m 755 power-on.sh ${D}/usr/local/bin/power-on.sh

  install -m 0644 mount_data1.service ${D}${systemd_system_unitdir}

  install -m 0644 setup_gpio.service ${D}${systemd_system_unitdir}

}

do_install_board() {
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
}

do_install:append() {
  do_install_board
}

FILES:${PN} += "${sysconfdir}"

SYSTEMD_SERVICE:${PN} += "mount_data1.service setup_gpio.service setup_i2c.service"

#Not needed for cloudripper
SYSTEMD_SERVICE:${PN}:remove = "enable_watchdog_ext_signal.service"
