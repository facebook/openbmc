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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
        file://bmc_cpld.cpp \
        file://bmc_cpld.h \
        file://platform.cpp \
        file://bios.h \
        file://bios.cpp \
        file://bmc_bios.cpp \
        "

LOCAL_URI:remove = " \
        file://bic_cpld.cpp \
        file://bic_cpld.h \
        file://bic_fw.cpp \
        file://bic_fw.h \
        file://bic_me.cpp \
        file://bic_me.h \
        file://bic_bios.cpp \
        file://bic_bios.h \
        "

DEPENDS += "libfpga libnetlakemtp-common libobmc-i2c libkv"
RDEPENDS:${PN} += "libfpga libnetlakemtp-common libobmc-i2c libkv"
LDFLAGS += "-lfpga -lnetlakemtp_common -lobmc-i2c"
