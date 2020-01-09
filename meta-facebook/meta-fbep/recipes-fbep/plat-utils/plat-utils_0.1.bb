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
           file://setup-gpio.sh \
           file://setup-i2c.sh \
           file://setup-emmc.sh \
           file://setup-usbnet.sh \
           file://setup-por.sh \
           file://sol-util \
           file://COPYING \
           file://workaround.sh \
          "

pkgdir = "utils"

S = "${WORKDIR}"

binfiles = "sol-util \
           "

RDEPENDS_${PN} += "gpiocli"
DEPENDS_append = "update-rc.d-native"

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

  # init
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  # the script to mount /mnt/data
  install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 59 5 .
  install -m 755 setup-i2c.sh ${D}${sysconfdir}/init.d/setup-i2c.sh
  update-rc.d -r ${D} setup-i2c.sh start 60 5 .
  install -m 755 setup-emmc.sh ${D}${sysconfdir}/init.d/setup-emmc.sh
  update-rc.d -r ${D} setup-emmc.sh start 05 S .
  install -m 755 setup-usbnet.sh ${D}${sysconfdir}/init.d/setup-usbnet.sh
  update-rc.d -r ${D} setup-usbnet.sh start 90 S .
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 70 S .
  install -m 755 workaround.sh ${D}${sysconfdir}/init.d/workaround.sh
  update-rc.d -r ${D} workaround.sh start 71 S .
}

FILES_${PN} += "/usr/local ${sysconfdir}"

RDEPENDS_${PN} = "bash"
