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
        file://plat/meson.build \
        file://bic_cxl.cpp \
        file://bic_cxl.h \
        file://bic_prot.cpp \
        file://bic_prot.hpp \
        file://bic_vr.cpp \
        file://bic_vr.h \
        file://bic_retimer.cpp \
        file://bic_retimer.h \
        file://bmc_cpld.cpp \
        file://bmc_cpld.h \
        file://expansion.cpp \
        file://expansion.h \
        file://mcu_fw.cpp \
        file://mcu_fw.h \
        file://mp5990.cpp \
        file://mp5990.h \
        file://nic_ext.cpp \
        file://nic_ext.h \
        file://platform.cpp \
        file://usbdbg.cpp \
        file://usbdbg.h \
        file://vpdb_vr.cpp \
        file://vpdb_vr.h \
        "

CXXFLAGS += "-DBIC_SUPPORT"
DEPENDS += "libbic libfpga libvr libfby35-common libncsi libnl-wrapper libobmc-i2c libmcu libprot"
