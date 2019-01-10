# Copyright 2018-present Facebook. All Rights Reserved.
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

SUMMARY = "Minipack CPLD drivers"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module kernel_extra_headers_export

PR = "r0"
PV = "0.1"

SRC_URI = "file://Makefile \
           file://domfpga.c \
           file://fcmcpld.c \
           file://iobfpga.c \
           file://pdbcpld.c \
           file://scmcpld.c \
           file://smbcpld.c \
           file://COPYING \
          "

S = "${WORKDIR}"

DEPENDS += "kernel-module-i2c-dev-sysfs"

RDEPENDS_${PN} += "kernel-module-i2c-dev-sysfs"

KERNEL_MODULE_AUTOLOAD += "                     \
 fcmcpld                                        \
 scmcpld                                        \
 smbcpld                                        \
 pdbcpld                                        \
 iobfpga                                        \
 domfpga                                        \
"
