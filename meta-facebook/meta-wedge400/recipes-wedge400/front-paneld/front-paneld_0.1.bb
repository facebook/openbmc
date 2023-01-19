#
# Copyright 2019-present Facebook. All Rights Reserved.
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
#

SUMMARY = "Front Panel Control Daemon"
DESCRIPTION = "Daemon to monitor and control the front panel "
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://front-paneld.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS:append = "liblog libpal libmisc-utils update-rc.d-native"
LDFLAGS = "-llog"

LOCAL_URI = " \
    file://Makefile \
    file://setup-front-paneld.sh \
    file://front-paneld.c \
    file://run-front-paneld.sh \
    "

binfiles = "front-paneld"

pkgdir = "front-paneld"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 front-paneld ${dst}/front-paneld
  ln -snf ../fbpackages/${pkgdir}/front-paneld ${bin}/front-paneld
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/front-paneld
  install -d ${D}${sysconfdir}/front-paneld
  install -m 755 setup-front-paneld.sh ${D}${sysconfdir}/init.d/setup-front-paneld.sh
  install -m 755 run-front-paneld.sh ${D}${sysconfdir}/sv/front-paneld/run
  #front-paneld should be run after sensord(91)
  update-rc.d -r ${D} setup-front-paneld.sh start 92 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/front-paneld ${prefix}/local/bin ${sysconfdir} "
RDEPENDS:${PN} += " libpal libbic libmisc-utils bash"
