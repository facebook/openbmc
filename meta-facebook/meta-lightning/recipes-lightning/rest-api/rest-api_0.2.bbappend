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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

CFLAGS += " -Wall -Werror "

S = "${WORKDIR}/rest-api-2"

SRC_URI += "file://plat_tree.py \
            file://node_api.py \
            file://node_bmc.py \
            file://node_fruid.py \
            file://node_sensors.py \
            file://node_logs.py \
            file://node_fans.py \
            file://node_health.py \
            file://node_peb.py \
            file://node_fcb.py \
            file://node_flash.py \
            file://node_pdpb.py \
            file://node.py \
            file://tree.py \
            file://pal.py \
            file://rest.cfg \
            file://rest_config.py \
            file://rest-api-1/run_rest \
            file://rest-api-2/rest.py \
            file://rest-api-2/setup-rest-api.sh \
            file://rest-api-2/rest_utils.py \
            file://rest-api-2/board_endpoint.py \
            file://rest-api-2/board_setup_routes.py \
            file://rest-api-2/boardroutes.py \
           "

DEPENDS_append = " update-rc.d-native"

binfiles = "node.py \
            tree.py \
            pal.py \
            plat_tree.py \
            node_api.py \
            node_peb.py \
            node_pdpb.py \
            node_fcb.py \
            node_bmc.py \
            node_fans.py \
            node_flash.py \
            node_fruid.py \
            node_health.py \
            node_logs.py \
            node_sensors.py \
            rest_config.py \
           "

binfiles2 += "rest.py \
              setup-rest-api.sh \
              rest_utils.py \
              board_endpoint.py \
              boardroutes.py \
              board_setup_routes.py \
              "

pkgdir = "rest-api"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles2}; do
    install -m 755 ${S}/$f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  for f in ${binfiles}; do
    install -m 755 ${WORKDIR}/$f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  for f in ${otherfiles}; do
    install -m 644 $f ${dst}/$f
  done
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/restapi
  install -m 755 ../rest-api-1/run_rest ${D}${sysconfdir}/sv/restapi/run
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 644 ../rest.cfg ${D}${sysconfdir}/rest.cfg
  install -m 755 setup-rest-api.sh ${D}${sysconfdir}/init.d/setup-rest-api.sh
  update-rc.d -r ${D} setup-rest-api.sh start 95 4 5  .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "

RDEPENDS_${PN} += " libpal "
