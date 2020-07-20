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

SRC_URI += "file://setup-fan.sh \
            file://fsc-config2s.json \
            file://fsc-config4s_M.json\
            file://fsc-config8s_M.json\
            file://fsc-config_Slave.json\
            file://zone1_2s.fsc \
            file://zone1_4s_M.fsc \
            file://zone1_8s_M.fsc \
            file://zone1_Slave.fsc \
            file://fsc_board.py \
           "

FSC_CONFIG += "fsc-config2s.json \
               fsc-config4s_M.json \
               fsc-config8s_M.json \
               fsc-config_Slave.json \
              " 

FSC_ZONE_CONFIG +="zone1_8s_M.fsc \
                   zone1_4s_M.fsc \
                   zone1_2s.fsc \
                   zone1_Slave.fsc \
                  "

FSC_INIT_FILE += "setup-fan.sh"
