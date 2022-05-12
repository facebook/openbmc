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

SUMMARY = "ELBERT CPLD drivers"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=03f05aa5b4ffe5d1c1439f76e36ac447"

inherit module kernel_extra_headers_export

PR = "r0"
PV = "0.1"

LOCAL_URI = " \
    file://Makefile \
    file://fancpld.c \
    file://smbcpld.c \
    file://scmcpld.c \
    file://COPYING \
    "

DEPENDS += "kernel-module-i2c-dev-sysfs"

RDEPENDS:${PN} += "kernel-module-i2c-dev-sysfs"

KERNEL_MODULE_AUTOLOAD += "                     \
 fancpld                                        \
 smbcpld                                        \
 scmcpld                                        \
"
