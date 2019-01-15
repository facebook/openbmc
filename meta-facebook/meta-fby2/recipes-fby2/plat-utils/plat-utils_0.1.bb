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
SUMMARY = "Utilities"
DESCRIPTION = "Various utilities"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

SRC_URI = "file://ast-functions \
           file://sol-util \
           file://power_led.sh \
           file://power_util.py \
           file://post_led.sh \
           file://reset_usb.sh \
           file://setup-gpio.sh \
           file://setup-sysconfig.sh \
           file://mdio.py \
           file://eth0_mac_fixup.sh \
           file://fby2_power.sh \
           file://power-on.sh \
           file://rc.local \
           file://src \
           file://COPYING \
           file://check_ocp_nic.sh \
           file://check_slot_type.sh \
           file://setup-platform.sh \
           file://hotservice-reinit.sh \
           file://check_server_type.sh \
           file://setup-server-type.sh \
           file://setup-por.sh \
           file://sync_date.sh \
           file://time-sync.sh \
           file://cpld-dump.sh \
           file://dump_cpld_ep.sh \
           file://dump_cpld_rc.sh \
           file://sboot-cpld-dump.sh \
          "

pkgdir = "utils"

S = "${WORKDIR}"

binfiles = "sol-util power_led.sh post_led.sh \
  reset_usb.sh mdio.py fby2_power.sh power_util.py \
  check_ocp_nic.sh check_slot_type.sh hotservice-reinit.sh check_server_type.sh time-sync.sh sync_date.sh \
  cpld-dump.sh dump_cpld_ep.sh dump_cpld_rc.sh sboot-cpld-dump.sh"

DEPENDS_append = "update-rc.d-native"
RDEPENDS_${PN} += "bash python3 "

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  install -d $dst
  install -m 644 ast-functions ${dst}/ast-functions
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}
  for f in ${binfiles}; do
      install -m 755 $f ${dst}/${f}
      ln -s ../fbpackages/${pkgdir}/${f} ${localbindir}/${f}
  done

  # common lib and include files
  install -d ${D}${includedir}/facebook
  install -m 0644 src/include/log.h ${D}${includedir}/facebook/log.h

  # init
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 59 5 .
  install -m 755 setup-sysconfig.sh ${D}${sysconfdir}/init.d/setup-sysconfig.sh
  update-rc.d -r ${D} setup-sysconfig.sh start 60 5 .
  install -m 755 setup-server-type.sh ${D}${sysconfdir}/init.d/setup-server-type.sh
  update-rc.d -r ${D} setup-server-type.sh start 66 5 .
  # networking is done after rcS, any start level within rcS
  # for mac fixup should work
  #install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
  #update-rc.d -r ${D} eth0_mac_fixup.sh start 70 S .
  install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
  update-rc.d -r ${D} power-on.sh start 70 5 .
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 63 5 .
  install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
  update-rc.d -r ${D} rc.local start 99 2 3 4 5 .
  install -m 755 setup-platform.sh ${D}${sysconfdir}/init.d/setup-platform.sh
  update-rc.d -r ${D} setup-platform.sh start 91 S .
}

FILES_${PN} += "/usr/local ${sysconfdir}"
