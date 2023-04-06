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

LOCAL_URI += "\
    file://plat/meson.build \
    file://ast_bic.cpp \
    file://ast_bic.h \
    file://bic_bios.cpp \
    file://bic_capsule.cpp \
    file://bic_capsule.h \
    file://bic_cpld_ext.cpp \
    file://bic_cpld_ext.h \
    file://bic_expansion_vr.cpp \
    file://bic_fw_ext.cpp \
    file://bic_fw_ext.h \
    file://bic_m2_dev.cpp \
    file://bic_m2_dev.h \
    file://bic_pcie_sw.cpp \
    file://bic_pcie_sw.h \
    file://bic_vr.cpp \
    file://bic_vr.h \
    file://bmc_cpld.cpp \
    file://bmc_cpld.h \
    file://bmc_cpld_capsule.cpp \
    file://bmc_cpld_capsule.h \
    file://expansion.cpp \
    file://expansion.h \
    file://mcu_fw.cpp \
    file://mcu_fw.h \
    file://nic_ext.cpp \
    file://nic_ext.h \
    file://platform.cpp \
    file://usbdbg.cpp \
    file://usbdbg.h \
    "

CXXFLAGS += " -DBIC_SUPPORT "
DEPENDS += " \
    libbic \
    libfby3-common \
    libfpga \
    libmcu \
    libnl-wrapper \
    "

RDEPENDS:${PN} += "libfby3-common"
