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

SUMMARY = "Rest API Daemon"
DESCRIPTION = "Daemon to handle RESTful interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://rest.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

DEPENDS_append = " update-rc.d-native"

SRC_URI = "file://rest-api-1/setup-rest-api.sh \
           file://rest-api-1/rest.py \
           file://rest-api-1/common_endpoint.py \
           file://board_endpoint.py \
           file://rest-api-1/rest_watchdog.py \
           file://rest_config.py \
           file://rest-api-1/run_rest \
           file://rest-api-1/run_watchdog \
           file://rest.cfg \
           file://rest-api-1/rest_bmc.py \
           file://rest-api-1/rest_fruid.py \
           file://rest-api-1/rest_gpios.py \
           file://rest-api-1/rest_server.py \
           file://rest-api-1/rest_sensors.py \
           file://rest-api-1/rest_modbus.py \
           file://rest-api-1/rest_slotid.py \
           file://rest-api-1/rest_psu_update.py \
           file://rest-api-1/rest_mTerm.py \
           file://rest-api-1/bmc_command.py \
           file://rest-api-1/eeprom_utils.py \
           file://rest-api-1/rest_fcpresent.py \
           file://rest-api-1/rest_helper.py \
           file://rest-api-1/rest_utils.py \
           file://board_setup_routes.py \
           file://boardroutes.py \
           file://rest-api-1/common_setup_routes.py \
          "

S = "${WORKDIR}/rest-api-1"

binfiles = "board_setup_routes.py \
            boardroutes.py \
            board_endpoint.py \
            rest_config.py \
           "

binfiles1 = "rest.py \
             setup-rest-api.sh \
             rest_watchdog.py \
             common_endpoint.py \
             rest_bmc.py \
             rest_fruid.py \
             rest_gpios.py \
             rest_server.py \
             rest_sensors.py \
             bmc_command.py \
             eeprom_utils.py \
             rest_modbus.py \
             rest_slotid.py \
             rest_psu_update.py \
             rest_fcpresent.py \
             rest_helper.py \
             rest_utils.py \
             rest_mTerm.py \
             common_setup_routes.py"

pkgdir = "rest-api"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
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
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/restapi
  install -m 755 run_rest ${D}${sysconfdir}/sv/restapi/run
  install -d ${D}${sysconfdir}/sv/restwatchdog
  install -m 755 run_watchdog ${D}${sysconfdir}/sv/restwatchdog/run
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 644 ${WORKDIR}/rest.cfg ${D}${sysconfdir}/rest.cfg
  install -m 755 setup-rest-api.sh ${D}${sysconfdir}/init.d/setup-rest-api.sh
  update-rc.d -r ${D} setup-rest-api.sh start 95 2 3 4 5  .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "
