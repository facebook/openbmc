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
        file://nic_ext.cpp \
        file://nic_ext.h \
        file://bmc_cpld.cpp \
        file://bmc_cpld.h \
        file://platform.cpp \
        file://expansion.h \
        file://expansion.cpp \
        file://bic_vr.h \
        file://bic_vr.cpp \
        file://bic_pcie_sw.cpp \
        file://bic_pcie_sw.h \
        file://bic_expansion_vr.cpp \
        file://usbdbg.h \
        file://usbdbg.cpp \
        file://mcu_fw.h \
        file://mcu_fw.cpp \
        file://bic_m2_dev.cpp \
        file://bic_m2_dev.h \
        "

CXXFLAGS += "-DBIC_SUPPORT"
DEPENDS += "libbic libfpga libvr libfby35-common libncsi libnl-wrapper libobmc-i2c libmcu libkv"
RDEPENDS:${PN} += "libbic libvr libfpga libfby35-common libnl-wrapper libobmc-i2c libmcu libkv"
LDFLAGS += "-lbic -lfpga -lvr -lfby35_common -lnl-wrapper -lobmc-i2c -lmcu"
