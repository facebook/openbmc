# Copyright 2015-present Facebook. All Rights Reserved.
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

SUMMARY = "Wedge100 i2c device library"
DESCRIPTION = "i2c device sysfs attribute library"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module kernel_extra_headers

PR = "r0"
PV = "0.1"

SRC_URI = "file://Makefile \
           file://i2c_dev_sysfs.c \
           file://i2c_dev_sysfs.h \
           file://COPYING \
          "

S = "${WORKDIR}"

LINUX_EXTRA_HEADERS = "i2c_dev_sysfs.h"

do_install() {
    module_do_install
    kernel_extra_headers_do_install
}
