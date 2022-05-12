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
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

LOCAL_URI = " \
    file://ast-functions \
    file://power-on.sh \
    file://sync_date.sh \
    file://COPYING \
    file://setup-dev.sh \
    file://sol-util \
    file://setup-sic.sh \
    file://check_eth0_ipv4.sh \
    file://recovery-bic-util \
    file://check_bmc_ready.sh \
    "

pkgdir = "utils"

# the tools for BMC will be installed in the image
binfiles = " sync_date.sh sol-util recovery-bic-util check_bmc_ready.sh "

DEPENDS:append = "update-rc.d-native"
RDEPENDS:${PN} += "bash python3 "

do_install() {
  # install the package dir
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  install -d $dst

  # install ast-functions
  install -m 644 ast-functions ${dst}/ast-functions

  # create linkages to those binaries
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}
  for f in ${binfiles}; do
      install -m 755 $f ${dst}/${f}
      ln -s ../fbpackages/${pkgdir}/${f} ${localbindir}/${f}
  done

  # install the directories
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d

  # install setup-dev.sh
  install -m 755 setup-dev.sh ${D}${sysconfdir}/init.d/setup-dev.sh
  update-rc.d -r ${D} setup-dev.sh start 60 5 .

  # install setup-gpio.sh
  # install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  # update-rc.d -r ${D} setup-gpio.sh start 59 5 .

  # install power-on.sh
  install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
  update-rc.d -r ${D} power-on.sh start 70 5 .

  # install check_eth0_ipv4.sh
  install -m 755 check_eth0_ipv4.sh ${D}${sysconfdir}/init.d/check_eth0_ipv4.sh
  update-rc.d -r ${D} check_eth0_ipv4.sh start 71 5 .

  # install setup-sic.sh
  install -m 755 setup-sic.sh ${D}${sysconfdir}/init.d/setup-sic.sh
  update-rc.d -r ${D} setup-sic.sh start 68 5 .

  # install check_bmc_ready.sh
  install -m 755 check_bmc_ready.sh ${D}${sysconfdir}/init.d/check_bmc_ready.sh
  update-rc.d -r ${D} check_bmc_ready.sh start 99 5 .
}

FILES:${PN} += "/usr/local ${sysconfdir}"
