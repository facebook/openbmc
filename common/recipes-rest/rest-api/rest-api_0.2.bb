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

inherit python3unittest
inherit ptest

SUMMARY = "Rest API Daemon"
DESCRIPTION = "Daemon to handle RESTful interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

SRC_URI = "file://rest.py \
           file://common_logging.py \
           file://common_tree.py \
           file://common_middlewares.py \
           file://common_auth.py\
           file://acl_config.py\
           file://acl_providers/__init__.py\
           file://acl_providers/common_acl_provider_base.py\
           file://acl_providers/dummy_acl_provider.py\
           file://plat_tree.py \
           file://rest_crawler.py \
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
           file://setup-rest-api.sh \
           file://setup_plat_routes.py \
           file://run_rest \
           file://test_node_sensors.py \
           file://test_auth_enforcer.py \
           file://test_common_logging.py \
           file://test_rest_config.py \
          "

S = "${WORKDIR}"
DEPENDS += "libpal update-rc.d-native aiohttp python3-psutil json-log-formatter-native"

do_compile_ptest() {
cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
python -m unittest discover /usr/local/fbpackages/rest-api
EOF
}

do_install_class-target() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  acld="${D}/usr/local/fbpackages/${pkgdir}/acl_providers"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -d $acld
  for f in ${S}/*.py; do
    n=$(basename $f)
    install -m 755 "$f" ${dst}/$n
    ln -snf ../fbpackages/${pkgdir}/$n ${bin}/$n
  done
  for f in ${S}/acl_providers/*.py; do
    n=$(basename $f)
    install -m 755 "$f" ${acld}/$n
    ln -snf ../fbpackages/${pkgdir}/acl_providers/$n ${bin}/$n
  done
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/restapi
  install -m 755 ${WORKDIR}/run_rest ${D}${sysconfdir}/sv/restapi/run
  install -m 644 ${WORKDIR}/rest.cfg ${D}${sysconfdir}/rest.cfg
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 ${WORKDIR}/setup-rest-api.sh ${D}${sysconfdir}/init.d/setup-rest-api.sh
  update-rc.d -r ${D} setup-rest-api.sh start 95 5 .
}


pkgdir = "rest-api"
RDEPENDS_${PN} += "libpal python3-core aiohttp json-log-formatter"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "
FILES_${PN}-ptest = "${libdir}/rest-api/ptest/run-ptest"
