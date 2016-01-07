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

SRC_URI = "file://setup-rest-api.sh \
           file://rest.py \
           file://rest_bmc.py \
           file://rest_fruid.py \
           file://rest_gpios.py \
           file://rest_server.py \
           file://rest_sensors.py \
           file://rest_modbus.py \
           file://rest_slotid.py \
           file://rest_psu_update.py \
           file://bmc_command.py \
           file://rest_usb2i2c_reset.py \
           file://loop_rest_server.sh \
          "

S = "${WORKDIR}"

binfiles = "rest.py rest_bmc.py rest_fruid.py rest_gpios.py rest_server.py rest_sensors.py bmc_command.py rest_modbus.py rest_slotid.py rest_psu_update.py rest_usb2i2c_reset.py setup-rest-api.sh loop_rest_server.sh "

pkgdir = "rest-api"

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
  update-rc.d -r ${D} setup-rest-api.sh start 95 2 3 4 5  .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rest-api ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories for the fand binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
