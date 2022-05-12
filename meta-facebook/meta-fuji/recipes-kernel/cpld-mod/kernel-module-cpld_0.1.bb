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

SUMMARY = "Fuji CPLD drivers"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module kernel_extra_headers_export

PR = "r0"
PV = "0.1"

LOCAL_URI = " \
    file://Makefile \
    file://fcbcpld.c \
    file://scmcpld.c \
    file://smb_debugcardcpld.c \
    file://smb_pwrcpld.c \
    file://smb_syscpld.c \
    file://COPYING \
    "

DEPENDS += "kernel-module-i2c-dev-sysfs"

RDEPENDS:${PN} += "kernel-module-i2c-dev-sysfs"

KERNEL_MODULE_AUTOLOAD += "                     \
 fcbcpld                                        \
 scmcpld                                        \
 smb_debugcardcpld                              \
 smb_pwrcpld                                    \
 smb_syscpld                                    \
"
