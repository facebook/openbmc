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
inherit ptest

SUMMARY = "Rest API Daemon"
DESCRIPTION = "Daemon to handle RESTful interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"
DEPENDS_append = " update-rc.d-native aiohttp-native json-log-formatter-native"

REST_API_RDEPENDS = "python3-core aiohttp json-log-formatter"
RDEPENDS_${PN} += "${REST_API_RDEPENDS}"
RDEPENDS_${PN}_class-target += "${REST_API_RDEPENDS} libgpio-ctrl"


SRC_URI = "file://setup-rest-api.sh \
           file://rest.py \
           file://common_utils.py \
           file://common_webapp.py \
           file://common_middlewares.py \
           file://common_logging.py \
           file://common_auth.py \
           file://async_ratelimiter.py \
           file://acl_config.py \
           file://acl_providers/__init__.py \
           file://acl_providers/cached_acl_provider.py \
           file://acl_providers/common_acl_provider_base.py \
           file://rest_config.py \
           file://rest_pal_legacy.py \
           file://node.py \
           file://node_bmc.py \
           file://vboot.py \
           file://run_rest \
           file://rest.cfg \
           file://rest_ntpstatus.py \
           file://rest_utils.py \
           file://rest_fscd_sensor_data.py \
           file://rest_modbus_cmd.py \
           file://test_rest_modbus_cmd.py \
           file://test_async_ratelimiter.py \
           file://test_auth_enforcer.py \
           file://test_cached_acl_provider.py \
           file://test_common_logging.py \
           file://test_rest_config.py \
           file://test_rest_fscd_sensor_data.py \
           file://test_common_auth.py \
           file://test_common_acl_provider_base.py \
           file://common_setup_routes.py \
           file://setup_plat_routes.py \
           file://boot_source.py \
           file://restapi.service \
           file://board_setup_routes.py \
           file://redfish_common_routes.py \
           file://redfish_service_root.py \
           file://redfish_account_service.py \
           file://redfish_chassis.py \
           file://redfish_chassis_helper.py \
           file://redfish_managers.py \
           file://test_redfish_common_routes.py \
           file://redfish_base.py \
           file://test_redfish_root_controller.py \
           file://test_redfish_account_controller.py \
           file://test_redfish_managers_controller.py \
           file://test_redfish_chassis_controller.py \
        "

S = "${WORKDIR}"


aclfiles = "__init__.py \
            cached_acl_provider.py \
            common_acl_provider_base.py \
"

SRC_URI += "${@bb.utils.contains('MACHINE_FEATURES', 'compute-rest', \
           'file://compute_rest_shim.py \
            file://rest_crawler.py \
            file://node_attestation.py \
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
            file://node_fans.py \
            file://node_health.py \
            file://node_mezz.py \
            file://node_sensors.py \
            file://node_spb.py \
            file://node_config.py \
            file://node_fcb.py \
            file://node_identify.py \
            file://node_pdpb.py \
            file://node_e1s_iocm.py \
            file://node_uic.py \
            file://test_node_sensors.py \
            ', \
            'file://common_endpoint.py \
            file://board_endpoint.py \
            file://rest_watchdog.py \
            file://rest_bmc.py \
            file://rest_fruid.py \
            file://rest_fruid_pim.py \
            file://rest_piminfo.py \
            file://rest_gpios.py \
            file://rest_server.py \
            file://rest_sensors.py \
            file://rest_psu_update.py \
            file://eeprom_utils.py \
            file://rest_fcpresent.py \
            file://rest_helper.py \
            file://test_common_middlewares.py \
            file://test_rest_gpios.py \
            file://boardroutes.py\
            ', d)}"

RDEPENDS_${PN}_class-target += "${@bb.utils.contains('MACHINE_FEATURES', 'compute-rest', \
                  'libpal python3-psutil', '', d)}"

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
  acld="${D}/usr/local/fbpackages/${pkgdir}/acl_providers"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -d $acld
  install -d ${D}${sysconfdir}
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

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi

  install -m 644 ${WORKDIR}/rest.cfg ${D}${sysconfdir}/rest.cfg
}

do_compile_ptest() {
cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
  coverage erase
  coverage run -m unittest discover /usr/local/fbpackages/rest-api
  coverage report -m --omit="*/test*"
EOF
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "
BBCLASSEXTEND += "native nativesdk"

SYSTEMD_SERVICE_${PN} = "restapi.service"
