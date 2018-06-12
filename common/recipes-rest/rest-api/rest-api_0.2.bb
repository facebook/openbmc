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


SRC_URI = "file://rest.py \
           file://node.py \
           file://tree.py \
           file://pal.py \
           file://vboot.py \
           file://rest.cfg \
           file://rest_config.py \
           file://node_bmc.py \
           file://node_api.py \
           file://node_dpb.py \
           file://node_flash.py \
           file://node_iom.py \
           file://node_peb.py \
           file://node_server.py \
           file://node_bios.py \
           file://node_enclosure.py \
           file://node_fruid.py \
           file://node_logs.py \
           file://node_scc.py \
           file://node_sled.py \
           file://node_bmc.py \
           file://node_fans.py \
           file://node_health.py \
           file://node_mezz.py \
           file://node_sensors.py \
           file://node_spb.py \
           file://node_config.py \
           file://node_fcb.py \
           file://node_identify.py \
           file://node_pdpb.py \
           file://node_server_2s.py \
           file://setup-rest-api.sh \
           file://run_rest \
          "

S = "${WORKDIR}"
DEPENDS += "libpal update-rc.d-native"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${S}/*.py; do
    n=$(basename $f)
    install -m 755 "$f" ${dst}/$n
    ln -snf ../fbpackages/${pkgdir}/$n ${bin}/$n
  done
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/restapi
  install -m 755 run_rest ${D}${sysconfdir}/sv/restapi/run
  install -m 644 ${WORKDIR}/rest.cfg ${D}${sysconfdir}/rest.cfg
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-rest-api.sh ${D}${sysconfdir}/init.d/setup-rest-api.sh
  update-rc.d -r ${D} setup-rest-api.sh start 95 5 .
}

pkgdir = "rest-api"
RDEPENDS_${PN} += "libpal"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "
