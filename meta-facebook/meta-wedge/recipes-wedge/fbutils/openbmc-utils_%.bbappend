# Copyright 2014-present Facebook. All Rights Reserved.
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

SRC_URI += "file://board-utils.sh \
           file://us_console.sh \
           file://sol.sh \
           file://power_led.sh \
           file://post_led.sh \
           file://reset_usb.sh \
           file://reset_cp2112.sh \
           file://setup-gpio.sh \
           file://setup_rov.sh \
           file://eth0_mac_fixup.sh \
           file://wedge_power.sh \
           file://reset_brcm.sh \
           file://power-on.sh \
           file://wedge_us_mac.sh \
           file://setup_switch.py \
           file://create_vlan_intf \
           file://setup_i2c.sh \
           file://start_us_monitor.sh \
           file://us_monitor.sh \
          "

OPENBMC_UTILS_FILES += " \
  board-utils.sh us_console.sh sol.sh power_led.sh post_led.sh \
  reset_usb.sh setup_rov.sh wedge_power.sh wedge_us_mac.sh \
  setup_switch.py us_monitor.sh reset_brcm.sh reset_cp2112.sh \
  "

DEPENDS_append = " update-rc.d-native"
RDEPENDS_${PN} += "python3-core"

do_install_board() {
  # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
  olddir="/usr/local/fbpackages/utils"
  install -d ${D}${olddir}
  ln -s "/usr/local/bin/openbmc-utils.sh" ${D}${olddir}/ast-functions

  # init
  install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 59 S .
  # setup i2c and sensors
  install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
  update-rc.d -r ${D} setup_i2c.sh start 60 S .
  # create VLAN intf automatically
  install -d ${D}/${sysconfdir}/network/if-up.d
  install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf
  # networking is done after rcS, any start level within rcS
  # for mac fixup should work
  install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
  update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .
  install -m 755 start_us_monitor.sh ${D}${sysconfdir}/init.d/start_us_monitor.sh
  update-rc.d -r ${D} start_us_monitor.sh start 83 S .
  install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
  update-rc.d -r ${D} power-on.sh start 85 S .
}

do_install_append() {
  do_install_board
}

FILES_${PN} += "${sysconfdir}"
