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
SUMMARY = "Utilities"
DESCRIPTION = "Various utilities"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

LOCAL_URI = " \
    file://COPYING \
    file://ast-functions \
    file://setup-gpio.sh \
    file://sol-util \
    file://setup-i2c-dev.sh \
    file://setup-por.sh \
    "

pkgdir = "utils"


# the tools for BMC will be installed in the image
binfiles = " sol-util"

DEPENDS:append = "update-rc.d-native"
RDEPENDS:${PN} += "bash python3 gpiocli "

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

  # init
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 62 5 .
# setup-gpio.sh
  install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 59 5 .
# setup-i2c-dev.sh
  install -m 755 setup-i2c-dev.sh ${D}${sysconfdir}/init.d/setup-i2c-dev.sh
  update-rc.d -r ${D} setup-i2c-dev.sh start 60 5 .


# install check_eth0_ipv4.sh
#  install -m 755 ifup.sh ${D}${sysconfdir}/init.d/ifup.sh
#  update-rc.d -r ${D} ifup.sh start 100 5 .
}

FILES:${PN} += "/usr/local ${sysconfdir}"
