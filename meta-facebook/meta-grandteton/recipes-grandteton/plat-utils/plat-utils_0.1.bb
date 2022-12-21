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
    file://setup-gpio-common.sh \
    file://setup-gpio-cover.sh \
    file://sol-util \
    file://setup-common-dev.sh \
    file://setup-cover-dev.sh \
    file://setup-server-uart.sh \
    file://setup-por.sh \
    file://sync_date.sh \
    file://setup-lmsensors-cfg.sh \
    file://setup-usbnet.sh \
    file://setup-snr-mon.sh \
    "

pkgdir = "utils"


# the tools for BMC will be installed in the image
binfiles = " sol-util setup-server-uart.sh setup-usbnet.sh"

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

# setup-gpio-common.sh
  install -m 755 setup-gpio-common.sh ${D}${sysconfdir}/init.d/setup-gpio-common.sh
  update-rc.d -r ${D} setup-gpio-common.sh start 55 5 .
# setup-gpio-cover.sh
  install -m 755 setup-gpio-cover.sh ${D}${sysconfdir}/init.d/setup-gpio-cover.sh
  update-rc.d -r ${D} setup-gpio-cover.sh start 56 5 .
# setup-common-dev.sh
  install -m 755 setup-common-dev.sh ${D}${sysconfdir}/init.d/setup-common-dev.sh
  update-rc.d -r ${D} setup-common-dev.sh start 59 5 .
# setup-cover-dev.sh
  install -m 755 setup-cover-dev.sh ${D}${sysconfdir}/init.d/setup-cover-dev.sh
  update-rc.d -r ${D} setup-cover-dev.sh start 60 5 .
# setup-por.sh
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 63 5 .
# sync_date.sh
  install -m 755 sync_date.sh ${D}${sysconfdir}/init.d/sync_date.sh
  update-rc.d -r ${D} sync_date.sh start 66 5 .
# setup-lmsensors-cfg.sh
  install -m 755 setup-lmsensors-cfg.sh ${D}${sysconfdir}/init.d/setup-lmsensors-cfg.sh
  update-rc.d -r ${D} setup-lmsensors-cfg.sh start 68 5 .
}

FILES:${PN} += "/usr/local ${sysconfdir}"
