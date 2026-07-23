# Copyright 2020-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/plat_conf:"

SRC_URI += "file://netlakenext.cfg \
            file://Add-PCA9641-driver.patch \
            file://Fix-AST2600A1-i2c-irq-issue.patch \
            file://Revise-peci-driver-for-icelake-d.patch \
            file://1001-sbtsi-update-label-for-hwmon-entry-to-include-socket.patch \
            file://1002-sbrmi-update-label-for-hwmon-entry-to-include-socket.patch \
            file://1003-From-3c522d716254ec344c1c10308ee80ad7c5d4a0ee-Mon-Se.patch \
            file://1004-sbrmi-Validate-the-original-mailbox-command-ID-with-.patch \
            file://1005-sbrmi-Add-revision-3.1-support-for-register-size-det.patch \
            file://1006-sbrmi-CPUID-support-for-v1.0.patch \
            file://1007-sbrmi-Extend-clean-hwmon-teardown-in-RMI-driver.patch \
            file://1008-sbtsi-Ensure-clean-teardown-for-hwmon-and-IOCTL.patch \
            file://1101-kernel-5.10-prioritize-1-byte-register-address-probi.patch \
	"
