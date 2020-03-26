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
           file://setup-gpio.sh \
           file://setup_rov.sh \
           file://mdio.py \
           file://power-on.sh \
           file://src \
           file://COPYING \
           file://check_pal_sku.sh \
           file://setup-platform.sh \
           file://setup-por.sh \
          "

pkgdir = "utils"

S = "${WORKDIR}"

binfiles = "sol-util power_led.sh post_led.sh \
  mdio.py setup_rov.sh power_util.py \
  check_pal_sku.sh "

DEPENDS_append = "update-rc.d-native"
RDEPENDS_${PN} += "bash"
RDEPENDS_${PN} += "python3-core"
RDEPENDS_${PN} += "gpiocli"

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
  install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
  update-rc.d -r ${D} power-on.sh start 70 5 .
  install -m 755 setup-platform.sh ${D}${sysconfdir}/init.d/setup-platform.sh
  update-rc.d -r ${D} setup-platform.sh start 63 5 .
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 63 5 .
}

FILES_${PN} += "/usr/local ${sysconfdir}"
