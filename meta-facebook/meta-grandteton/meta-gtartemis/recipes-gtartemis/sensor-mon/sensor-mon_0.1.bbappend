# Copyright 2022-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
CFLAGS += "-DSENSOR_FAIL_DETECT"
SENSORD_MONITORED_FRUS = "mb nic0 nic1 vpdb hpdb fan_bp1 fan_bp2 scm cb mc mc_cxl1 mc_cxl2 mc_cxl3 mc_cxl4 mc_cxl5 mc_cxl6 mc_cxl7 mc_cxl8 cb_accl1 cb_accl2 cb_accl3 cb_accl4 cb_accl5 cb_accl6 cb_accl7 cb_accl8 cb_accl9 cb_accl10 cb_accl11 cb_accl12"
