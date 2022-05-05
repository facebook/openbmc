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
LOCAL_URI += " \
    file://init_pwm.sh \
    file://setup-fan.sh \
    file://FSC_BC_Type5_MP_v4_config.json \
    file://FSC_BC_Type5_MP_v2_zone0.fsc \
    file://FSC_BC_Type5_MP_v2_zone1.fsc \
    file://FSC_BC_Type7_MP_v4_config.json \
    file://FSC_BC_Type7_MP_v1_zone0.fsc \
    file://FSC_BC_Type7_MP_v1_zone1.fsc \
    file://fsc_board.py \
    file://setup_fscd_sensor.sh \
    file://check_M2_nvme.sh \
    "

FSC_BIN_FILES += "init_pwm.sh"

FSC_CONFIG += "FSC_BC_Type5_MP_v4_config.json \
			         FSC_BC_Type7_MP_v4_config.json \
              "

FSC_ZONE_CONFIG += "FSC_BC_Type5_MP_v2_zone0.fsc \
                    FSC_BC_Type5_MP_v2_zone1.fsc \
                    FSC_BC_Type7_MP_v1_zone0.fsc \
           			    FSC_BC_Type7_MP_v1_zone1.fsc \
           		   "

FSC_INIT_FILE += "setup-fan.sh"

do_install:append() {
  install -d ${D}${sysconfdir}
  install -d ${D}${sysconfdir}/init.d
  
  install -m 755 setup_fscd_sensor.sh ${D}${sysconfdir}/init.d/setup_fscd_sensor.sh
  update-rc.d -r ${D} setup_fscd_sensor.sh start 99 5 .

  install -m 755 check_M2_nvme.sh ${D}${sysconfdir}
}
