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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://bios.h \
            file://bios.cpp \
            file://platform.cpp \
            file://cpld.cpp \
            file://mcu_fw.h \
            file://mcu_fw.cpp \
            file://me.cpp \
            file://usbdbg.h \
            file://usbdbg.cpp \
            file://vr.cpp \
            file://fscd.cpp \
           "

DEPENDS += "libmcu libnm libpal libfpga libast-jtag libvr"
RDEPENDS_${PN} += "libmcu libnm libpal libfpga libast-jtag libvr"
LDFLAGS += "-lmcu -lnm -lpal -lfpga -last-jtag -lvr"
