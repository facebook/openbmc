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
SUMMARY = "Rest API Daemon"
DESCRIPTION = "Daemon to handle RESTful interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://rest.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

DEPENDS_append = " update-rc.d-native"

S = "${WORKDIR}"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://setup-rest-api.sh \
           file://plat_tree.py \
           file://node_api.py \
           file://node_spb.py \
           file://node_mezz.py \
           file://node_bmc.py \
           file://node_server.py \
           file://node_fruid.py \
           file://node_sensors.py \
           file://node_logs.py \
           file://node_config.py \
           file://pal.py \
          "

binfiles += "setup-rest-api.sh plat_tree.py node_api.py node_spb.py node_mezz.py node_bmc.py node_server.py node_fruid.py node_sensors.py node_logs.py node_config.py pal.py"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  for f in ${otherfiles}; do
    install -m 644 $f ${dst}/$f
  done
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-rest-api.sh ${D}${sysconfdir}/init.d/setup-rest-api.sh
  update-rc.d -r ${D} setup-rest-api.sh start 95 5  .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "
