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
    file://setup-gpio.sh \
    file://sync_date.sh \
    file://sol-util \
    file://setup-por.sh \
    file://setup-dev.sh \
    file://plat-functions \
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

  # setup-gpio.sh
    install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
    update-rc.d -r ${D} setup-gpio.sh start 59 5 .

  # sync_date.sh
    install -m 755 sync_date.sh ${D}${sysconfdir}/init.d/sync_date.sh
    update-rc.d -r ${D} sync_date.sh start 66 5 .

  # setup-por.sh
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 67 5 .

  # install plat-functions
  install -m 644 plat-functions ${dst}/plat-functions
}

FILES:${PN} += "/usr/local ${sysconfdir}"
