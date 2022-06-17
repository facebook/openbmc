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
    file://setup-gpio.sh \
    file://setup-switch.sh \
    file://check-i2c-pwr.sh \
    file://setup-i2c.sh \
    file://setup-usbnet.sh \
    file://setup-por.sh \
    file://sol-util \
    file://sync-rtc.sh \
    file://run-sync-rtc.sh \
    file://COPYING \
    file://workaround.sh \
    file://check-vr-ic.sh \
    file://check-switch.sh \
    "

pkgdir = "utils"


binfiles = "sol-util \
           "

RDEPENDS:${PN} += "gpiocli"
DEPENDS:append = "update-rc.d-native"

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
  update-rc.d -r ${D} setup-gpio.sh start 63 5 .
  install -m 755 setup-switch.sh ${D}${sysconfdir}/init.d/setup-switch.sh
  update-rc.d -r ${D} setup-switch.sh start 66 5 .
  install -m 755 setup-i2c.sh ${D}${sysconfdir}/init.d/setup-i2c.sh
  install -m 755 check-i2c-pwr.sh ${D}${sysconfdir}/init.d/check-i2c-pwr.sh
  update-rc.d -r ${D} check-i2c-pwr.sh start 67 5 .
  install -m 755 setup-usbnet.sh ${D}${sysconfdir}/init.d/setup-usbnet.sh
  update-rc.d -r ${D} setup-usbnet.sh start 10 5 .
  install -m 755 setup-por.sh ${D}${sysconfdir}/init.d/setup-por.sh
  update-rc.d -r ${D} setup-por.sh start 70 S .
  install -m 755 sync-rtc.sh ${D}${sysconfdir}/init.d/sync-rtc.sh
  install -m 755 run-sync-rtc.sh ${D}${sysconfdir}/init.d/run-sync-rtc.sh
  update-rc.d -r ${D} run-sync-rtc.sh start 99 5 .
  install -m 755 workaround.sh ${D}${sysconfdir}/init.d/workaround.sh
  update-rc.d -r ${D} workaround.sh start 71 S .
  install -m 755 check-vr-ic.sh ${D}${sysconfdir}/init.d/check-vr-ic.sh
  update-rc.d -r ${D} check-vr-ic.sh start 70 5 .
  install -m 755 check-switch.sh ${D}${sysconfdir}/init.d/check-switch.sh
  update-rc.d -r ${D} check-switch.sh start 69 5 .
}

FILES:${PN} += "/usr/local ${sysconfdir}"

RDEPENDS:${PN} = "bash"
