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
SUMMARY = "SMS KCS Daemon"
DESCRIPTION = "Daemon to handle SMS KCS interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://sms-kcsd.c;beginline=12;endline=24;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS:append = " update-rc.d-native"

LDFLAGS += "-lipmi -lobmc-i2c -llog -lmisc-utils"
DEPENDS += "libipmi libobmc-i2c liblog libmisc-utils"
RDEPENDS:${PN} += "libipmi libobmc-i2c liblog libmisc-utils"

LOCAL_URI = " \
    file://Makefile \
    file://setup-sms-kcs.sh \
    file://sms-kcsd.c \
    file://alert_control.c \
    file://alert_control.h \
    "


binfiles = "sms-kcsd"

pkgdir = "sms-kcsd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 sms-kcsd ${dst}/sms-kcsd
  ln -snf ../fbpackages/${pkgdir}/sms-kcsd ${bin}/sms-kcsd
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-sms-kcs.sh ${D}${sysconfdir}/init.d/setup-sms-kcs.sh
  update-rc.d -r ${D} setup-sms-kcs.sh start 65 S .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/sms-kcsd ${prefix}/local/bin ${sysconfdir} "
