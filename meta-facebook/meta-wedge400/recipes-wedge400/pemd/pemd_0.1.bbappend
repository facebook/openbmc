# Copyright 2021-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://setup-pemd.sh \
            file://run-pemd.sh \
            file://platform_pemd.h \
            file://platform_pemd.c \
          "

S = "${WORKDIR}"

binfiles = "pemd \
           "

CFLAGS += " -llog -lpal -lpem "

DEPENDS += " liblog libpal libpem update-rc.d-native "
RDEPENDS:${PN} += " liblog libpal libpem "

pkgdir = "pemd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/pemd
  install -d ${D}${sysconfdir}/pemd
  install -m 755 setup-pemd.sh ${D}${sysconfdir}/init.d/setup-pemd.sh
  install -m 755 run-pemd.sh ${D}${sysconfdir}/sv/pemd/run
  update-rc.d -r ${D} setup-pemd.sh start 95 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/pemd ${prefix}/local/bin ${sysconfdir} "
