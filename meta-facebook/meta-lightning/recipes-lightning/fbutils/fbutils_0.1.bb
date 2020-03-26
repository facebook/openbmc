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
           file://power_led.sh \
           file://post_led.sh \
           file://pcie_switch.py \
           file://ssd_sku.sh \
           file://ssd_vid.sh \
           file://setup_adc.sh \
           file://disable_wdt2.sh\
           file://src \
           file://COPYING \
          "

pkgdir = "utils"

S = "${WORKDIR}"

binfiles = "power_led.sh post_led.sh"

DEPENDS_append = "update-rc.d-native"

do_install() {
  # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
  olddir="/usr/local/fbpackages/utils"
  install -d ${D}${olddir}
  ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

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
  # the script to set ADC
  install -m 755 setup_adc.sh ${D}${sysconfdir}/init.d/setup_adc.sh
  update-rc.d -r ${D} setup_adc.sh start 90 5 .
  install -m 755 pcie_switch.py ${D}${sysconfdir}/init.d/pcie_switch.py
  update-rc.d -r ${D} pcie_switch.py start 50 5 .
  install -m 755 ssd_sku.sh ${D}${sysconfdir}/init.d/ssd_sku.sh
  update-rc.d -r ${D} ssd_sku.sh start 51 5 .
  install -m 755 ssd_vid.sh ${D}${sysconfdir}/init.d/ssd_vid.sh
  update-rc.d -r ${D} ssd_vid.sh start 52 5 .
  install -m 755 disable_wdt2.sh ${D}${sysconfdir}/init.d/disable_wdt2.sh
  update-rc.d -r ${D} disable_wdt2.sh start 60 5 .
}

FILES_${PN} += "/usr/local ${sysconfdir}"
RDEPENDS_${PN} += " bash"
