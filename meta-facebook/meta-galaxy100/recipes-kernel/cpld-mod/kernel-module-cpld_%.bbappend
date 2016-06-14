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

SUMMARY = "Galaxy100 CPLD drivers"
LICENSE = "GPLv2"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

PR = "r0"
PV = "0.1"

SRC_URI = "file://Makefile \
           file://i2c_dev_sysfs.c \
           file://i2c_dev_sysfs.h \
           file://syscpld.c \
           file://scmcpld.c \
           file://COPYING \
          "

S = "${WORKDIR}"

KERNEL_MODULE_AUTOLOAD = "                     \
 syscpld                                        \
 scmcpld                                        \
"
