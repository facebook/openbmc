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
SENSORD_MONITORED_FRUS = "mb nic0 nic1 vpdb hpdb fan_bp1 fan_bp2 scm acb meb meb_jcn1 meb_jcn2 meb_jcn3 meb_jcn4 meb_jcn9 meb_jcn10 meb_jcn11 meb_jcn12 meb_jcn13 meb_jcn14 acb_accl1 acb_accl2 acb_accl3 acb_accl4 acb_accl5 acb_accl6 acb_accl7 acb_accl8 acb_accl9 acb_accl10 acb_accl11 acb_accl12"
