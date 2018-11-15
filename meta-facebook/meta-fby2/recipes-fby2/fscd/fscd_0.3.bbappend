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
            file://FSC_FBY2_PVT_4TL_config.json \
            file://FSC_FBY2_PVT_4TL_zone1.fsc \
            file://FSC_FBY2_PVT_2GP_2TL_config.json \
            file://FSC_FBY2_PVT_2GP_2TL_zone1.fsc \
            file://FSC_FBY2_PVT_2CF_2TL_config.json \
            file://FSC_FBY2_PVT_2CF_2TL_zone1.fsc \
            file://FSC_FBRC_DVT_4RC_config.json \
            file://FSC_FBRC_DVT_4RC_zone1.fsc \
            file://FSC_FBEP_DVT_4EP_config.json \
            file://FSC_FBEP_DVT_4EP_zone1.fsc \
            file://FSC_FBEP_DVT_2GP_2EP_config.json \
            file://FSC_FBEP_DVT_2GP_2EP_zone1.fsc \
            file://FSC_FBGPV2_EVT_config.json \
            file://FSC_FBGPV2_EVT_zone1.fsc \
            file://fsc_board.py \
           "

FSC_BIN_FILES += "init_pwm.sh"

FSC_CONFIG += "FSC_FBY2_PVT_4TL_config.json \
               FSC_FBY2_PVT_2GP_2TL_config.json \
               FSC_FBY2_PVT_2CF_2TL_config.json \
               FSC_FBRC_DVT_4RC_config.json \
               FSC_FBEP_DVT_4EP_config.json \
               FSC_FBEP_DVT_2GP_2EP_config.json \
               FSC_FBGPV2_EVT_config.json \
              "

FSC_ZONE_CONFIG +="FSC_FBY2_PVT_4TL_zone1.fsc \
                   FSC_FBY2_PVT_2GP_2TL_zone1.fsc \
                   FSC_FBY2_PVT_2CF_2TL_zone1.fsc \
                   FSC_FBRC_DVT_4RC_zone1.fsc \
                   FSC_FBEP_DVT_4EP_zone1.fsc \
                   FSC_FBEP_DVT_2GP_2EP_zone1.fsc \
                   FSC_FBGPV2_EVT_zone1.fsc \
                  "

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 setup-fan.sh ${D}${sysconfdir}/init.d/setup-fan.sh
  update-rc.d -r ${D} setup-fan.sh start 92 5 .
}
