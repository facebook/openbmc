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
#

SUMMARY = "Terminal Multiplexer"
DESCRIPTION = "Util for multiplexing terminal"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://mTerm_server.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://mTerm_server.c \
           file://mTerm_client.c \
           file://mTerm_helper.c \
           file://mTerm_helper.h \
           file://tty_helper.c \
           file://tty_helper.h \
           file://Makefile \
           file://mTerm/run \
           file://mTerm-service-setup.sh \
          "

S = "${WORKDIR}"

CONS_BIN_FILES = "mTerm_server \
                  mTerm_client \
                 "
# If the target system has more than one service, 
# It may override this variable in it's bbappend
# file. Also the platform must provide those in
# its appended package.
MTERM_SERVICES = "mTerm \
                 "
pkgdir = "mTerm"

DEPENDS += "update-rc.d-native"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${CONS_BIN_FILES}; do
     install -m 755 $f ${dst}/$f
     ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  for svc in ${MTERM_SERVICES}; do
    install -d ${D}${sysconfdir}/sv/${svc}
    install -d ${D}${sysconfdir}/${svc}
    install -m 755 ${svc}/run ${D}${sysconfdir}/sv/${svc}/run
  done
  install -m 755 mTerm-service-setup.sh ${D}${sysconfdir}/init.d/mTerm-service-setup.sh
  update-rc.d -r ${D} mTerm-service-setup.sh start 84 S .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN}-dbg += "${FBPACKAGEDIR}/mTerm/.debug"
FILES_${PN} += "${FBPACKAGEDIR}/mTerm ${prefix}/local/bin ${sysconfdir}"
