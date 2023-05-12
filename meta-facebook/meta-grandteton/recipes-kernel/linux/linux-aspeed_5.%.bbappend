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

SRC_URI += "\
            file://grandteton.cfg \
            file://0005-ASD-Add-Shift-IR-DR-from-Exit-IR-DR-for-HW2-JTAG-xfe.patch \
	    file://0006-change-bitbang-to-single-bit.patch \
	"

KERNEL_MODULE_AUTOLOAD += " \
			   "
