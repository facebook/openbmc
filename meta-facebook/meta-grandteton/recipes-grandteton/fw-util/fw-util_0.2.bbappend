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
    file://usbdbg.h \
    file://usbdbg.cpp \
    file://mcu_fw.h \
    file://mcu_fw.cpp \
    file://bios.h \
    file://bios.cpp \
    file://plat_common.cpp \
    file://plat_cover.cpp \
    file://cpld.hpp \
    file://cpld.cpp \
    file://me.cpp \
    file://nic_mctp.cpp \
    file://nic_mctp.h \
    file://nic.cpp \
    file://nic.h \
    file://mezz_nic.hpp \
    file://mezz_nic.cpp \
    file://nic_ext.h \
    file://nic_ext.cpp \
    file://vr_fw.h \
    file://vr_fw.cpp \
    file://swb_common.cpp \
    file://swb_common.hpp \
    file://swb_cover.cpp \
    file://vpdb_common.cpp \
    file://hgx.cpp \
    file://signed_info.hpp \
    file://retimer.cpp \
    "

DEPENDS += " \
    libbic \
    libfpga \
    libgpio-ctrl \
    libhgx \
    libipmi \
    libmcu \
    libncsi \
    libnl-wrapper \
    libnm \
    libobmc-mctp \
    libpldm-oem \
    libvr \
    libpal \
    libpldm \
    fmt \
    "
