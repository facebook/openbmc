# Copyright 2015-present Facebook. All Rights Reserved.
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

SUMMARY = "GPIO Monitoring Daemon"
DESCRIPTION = "Daemon for monitoring the gpio signals"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://gpiod.cpp;beginline=4;endline=16;md5=3598c23c3531e1f059568c83d4174cc0"

LOCAL_URI = " \
    file://meson.build \
    file://gpiod.cpp \
    file://gpiod.h \
    file://gpiod_cover.cpp \
    file://setup-gpiod.sh \
    file://run-gpiod.sh \
    file://dump-nv-reg.sh \
    "



inherit meson pkgconfig
inherit legacy-packages

pkgdir = "gpiod"

DEPENDS += " \
    libpal \
    libgpio-ctrl \
    update-rc.d-native \
    libnm \
    libkv \
    libobmc-i2c \
    libpeci-sensors \
    libpldm-oem \
    libhgx \
"
RDEPENDS:${PN} = "bash"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/gpiod

  install -m 755 ${S}/run-gpiod.sh ${D}${sysconfdir}/sv/gpiod/run
  install -m 755 ${S}/setup-gpiod.sh ${D}${sysconfdir}/init.d/setup-gpiod.sh
  install -m 755 ${S}/dump-nv-reg.sh ${D}/usr/local/bin/dump-nv-reg.sh
  update-rc.d -r ${D} setup-gpiod.sh start 92 5 .
}
