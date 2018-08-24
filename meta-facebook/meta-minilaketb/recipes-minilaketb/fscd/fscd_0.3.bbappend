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

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://init_pwm.sh \
            file://setup-fan.sh \
            file://FSC_MINILAKETB_DVT.json \
            file://FSC_MINILAKETB_DVT.fsc \
            file://fsc_board.py \
           "

FSC_BIN_FILES += "init_pwm.sh"

FSC_CONFIG += "FSC_MINILAKETB_DVT.json \
              "

FSC_ZONE_CONFIG +="FSC_MINILAKETB_DVT.fsc \
                  "

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 setup-fan.sh ${D}${sysconfdir}/init.d/setup-fan.sh
  update-rc.d -r ${D} setup-fan.sh start 92 5 .
}
