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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "\
        file://nic_ext.cpp \
        file://nic_ext.h \
        file://bmc_fpga.cpp \
        file://bmc_fpga.h \
        file://platform.cpp \
        file://server.h \
        file://bic_cpld.cpp \
        file://bic_vr.cpp \
        file://bic_vr.h \
        "

CXXFLAGS += " -DBIC_SUPPORT "
DEPENDS += " libpal libbic libncsi libnl-wrapper libkv libfpga libfbgc-common "
RDEPENDS_${PN} += " libpal libbic libncsi libnl-wrapper libkv libfpga libfbgc-common "
LDFLAGS += " -lpal -lbic -lnl-wrapper -lkv -lfpga "
