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

inherit python3unittest
inherit systemd

SUMMARY = "Rest API Daemon"
DESCRIPTION = "Daemon to handle RESTful interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"
DEPENDS_append = " update-rc.d-native aiohttp-native json-log-formatter-native libgpio-ctrl"
RDEPENDS_${PN} += "python3-core aiohttp json-log-formatter"


SRC_URI = "file://setup-rest-api.sh \
           file://rest.py \
           file://common_endpoint.py \
           file://common_middlewares.py\
           file://common_logging.py\
           file://common_auth.py\
           file://acl_config.py\
           file://acl_providers/__init__.py\
           file://acl_providers/cached_acl_provider.py\
           file://acl_providers/common_acl_provider_base.py\
           file://acl_providers/dummy_acl_provider.py\
           file://board_endpoint.py \
           file://rest_watchdog.py \
           file://rest_config.py \
           file://pal.py \
           file://node.py \
           file://node_bmc.py \
           file://vboot.py \
           file://run_rest \
           file://rest.cfg \
           file://rest_bmc.py \
           file://rest_fruid.py \
           file://rest_fruid_pim.py \
           file://rest_piminfo.py \
           file://rest_gpios.py \
           file://rest_server.py \
           file://rest_sensors.py \
           file://rest_slotid.py \
           file://rest_psu_update.py \
           file://rest_mTerm.py \
           file://eeprom_utils.py \
           file://rest_fcpresent.py \
           file://rest_ntpstatus.py \
           file://rest_helper.py \
           file://rest_utils.py \
           file://board_setup_routes.py \
           file://test_auth_enforcer.py \
           file://test_cached_acl_provider.py \
           file://test_common_middlewares.py \
           file://test_common_logging.py \
           file://test_rest_config.py \
           file://boardroutes.py \
           file://common_setup_routes.py \
           file://setup_plat_routes.py \
           file://restapi.service \
          "

S = "${WORKDIR}"

binfiles = "acl_config.py\
            board_setup_routes.py \
            boardroutes.py \
            board_endpoint.py \
            common_auth.py \
            common_middlewares.py \
            common_logging.py \
            rest_config.py \
            node.py \
            node_bmc.py \
            rest.py \
            vboot.py \
            pal.py \
           "

binfiles1 = "setup-rest-api.sh \
             rest_watchdog.py \
             common_endpoint.py \
             rest_bmc.py \
             rest_fruid.py \
             rest_fruid_pim.py \
             rest_piminfo.py \
             rest_gpios.py \
             rest_server.py \
             rest_sensors.py \
             eeprom_utils.py \
             rest_slotid.py \
             rest_psu_update.py \
             rest_fcpresent.py \
             rest_ntpstatus.py \
             rest_helper.py \
             rest_utils.py \
             rest_mTerm.py \
             setup_plat_routes.py \
             common_setup_routes.py"

aclfiles = "__init__.py \
            cached_acl_provider.py \
            common_acl_provider_base.py \
            dummy_acl_provider.py"

pkgdir = "rest-api"


install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/restapi.service ${D}${systemd_system_unitdir}
}


install_sysv() {
    install -d ${D}${sysconfdir}/sv
    install -d ${D}${sysconfdir}/sv/restapi
    install -m 755 ${WORKDIR}/run_rest ${D}${sysconfdir}/sv/restapi/run
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 ${WORKDIR}/setup-rest-api.sh ${D}${sysconfdir}/init.d/setup-rest-api.sh
    update-rc.d -r ${D} setup-rest-api.sh start 95 2 3 4 5  .
}

do_install_class-target() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  acld="${D}/usr/local/fbpackages/${pkgdir}/acl_providers"
  install -d $dst
  install -d $bin
  install -d $acld
  install -d ${D}${sysconfdir}
  for f in ${binfiles1}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  for f in ${binfiles}; do
    install -m 755 ${WORKDIR}/$f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  for f in ${otherfiles}; do
    install -m 644 $f ${dst}/$f
  done
  for f in ${aclfiles}; do
    install -m 755 ${WORKDIR}/acl_providers/$f ${dst}/acl_providers/$f
  done
  install -m 644 ${WORKDIR}/rest.cfg ${D}${sysconfdir}/rest.cfg

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi
}


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "
BBCLASSEXTEND += "native nativesdk"

SYSTEMD_SERVICE_${PN} = "restapi.service"
