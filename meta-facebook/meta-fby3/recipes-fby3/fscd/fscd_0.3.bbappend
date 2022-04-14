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

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

LOCAL_URI += "\
    file://setup-fan.sh \
    file://FSC_CLASS2_EVT_config.json \
    file://FSC_CLASS2_EVT_zone1.fsc \
    file://FSC_CLASS1_PVT_type1.json \
    file://FSC_CLASS1_EVT_type10.json \
    file://FSC_CLASS1_EVT_type10_zone1.fsc \
    file://FSC_CLASS1_EVT_type1_zone1.fsc \
    file://FSC_CLASS1_type15.json \
    file://FSC_CLASS1_type15_zone1.fsc \
    file://FSC_CLASS1_DVT_EDSFF_1U.json \
    file://FSC_CLASS1_DVT_EDSFF_1U.fsc \
    file://FSC_CLASS2_PVT_SPE_config.json \
    file://FSC_CLASS2_DVT_SPE_zone1.fsc \
    file://FSC_CLASS1_CONFIG_D_GPV3.json \
    file://FSC_CLASS1_CONFIG_D_GPV3.fsc \
    file://FSC_CLASS1_EVT_DP.json \
    file://FSC_CLASS1_EVT_DP.fsc \
    file://FSC_CLASS1_DVT_DP_HBA.json \
    file://FSC_CLASS1_DVT_DP_HBA.fsc \
    file://FSC_CLASS1_DP_FAVA.json \
    file://FSC_CLASS1_DP_FAVA.fsc \
    file://FSC_CLASS2_DVT_CWC.json \
    file://FSC_CLASS2_DVT_CWC.fsc \
    "

FSC_CONFIG += "FSC_CLASS1_PVT_type1.json \
               FSC_CLASS1_EVT_type10.json \
               FSC_CLASS1_type15.json \
               FSC_CLASS2_EVT_config.json \
               FSC_CLASS1_DVT_EDSFF_1U.json \
               FSC_CLASS2_PVT_SPE_config.json \
               FSC_CLASS1_CONFIG_D_GPV3.json \
               FSC_CLASS1_EVT_DP.json \
               FSC_CLASS1_DVT_DP_HBA.json \
               FSC_CLASS1_DP_FAVA.json \
               FSC_CLASS2_DVT_CWC.json \
              "

FSC_ZONE_CONFIG += " FSC_CLASS2_EVT_zone1.fsc \
                     FSC_CLASS1_EVT_type10_zone1.fsc \
                     FSC_CLASS1_type15_zone1.fsc \
                     FSC_CLASS1_EVT_type1_zone1.fsc \
                     FSC_CLASS1_DVT_EDSFF_1U.fsc \
                     FSC_CLASS2_DVT_SPE_zone1.fsc \
                     FSC_CLASS1_CONFIG_D_GPV3.fsc \
                     FSC_CLASS1_EVT_DP.fsc \
                     FSC_CLASS1_DVT_DP_HBA.fsc \
                     FSC_CLASS1_DP_FAVA.fsc \
                     FSC_CLASS2_DVT_CWC.fsc \
                   "
FSC_INIT_FILE += "setup-fan.sh"
