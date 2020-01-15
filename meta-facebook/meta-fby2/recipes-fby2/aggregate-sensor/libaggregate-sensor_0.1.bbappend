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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://aggregate-sensor-conf.json \
            file://aggregate-sensor-gpv2-conf.json \
            file://aggregate-sensor-gpv2-10kfan-conf.json \
            file://aggregate-sensor-fbnd-conf.json \
            file://aggregate-sensor-yv250-10kfan-conf.json \
            file://aggregate-sensor-yv250-15kfan-conf.json \
           "

do_install_append() {
  install -d ${D}${sysconfdir}
  install -m 644 ${WORKDIR}/aggregate-sensor-gpv2-conf.json ${D}${sysconfdir}/aggregate-sensor-gpv2-conf.json
  install -m 644 ${WORKDIR}/aggregate-sensor-gpv2-conf.json ${D}${sysconfdir}/aggregate-sensor-gpv2-10kfan-conf.json
  install -m 644 ${WORKDIR}/aggregate-sensor-fbnd-conf.json ${D}${sysconfdir}/aggregate-sensor-fbnd-conf.json
  install -m 644 ${WORKDIR}/aggregate-sensor-yv250-10kfan-conf.json ${D}${sysconfdir}/aggregate-sensor-yv250-10kfan-conf.json
  install -m 644 ${WORKDIR}/aggregate-sensor-yv250-15kfan-conf.json ${D}${sysconfdir}/aggregate-sensor-yv250-15kfan-conf.json
}
