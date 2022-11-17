# Copyright 2022-present Facebook. All Rights Reserved.
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

SUMMARY = "pldm daemon"
DESCRIPTION = "The pldm daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pldmd.cpp;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

RDEPENDS:${PN} = " libpldm libpldm-oem glog"
DEPENDS = " libpldm libpldm-oem cli11 update-rc.d-native glog"


LOCAL_URI = " \
    file://meson.build \
    file://pldmd.cpp \
    file://fd_handler.hpp \
    file://fd_handler.cpp \
    file://instance_id.hpp \
    file://instance_id.cpp \
    file://pldm_fd_handler.hpp \
    file://pldm_fd_handler.cpp \
    file://vector_handler.hpp \
    file://pldmd-util.cpp \
    "
pkgdir = "pldmd"

FILES:${PN} = "${prefix}/local/bin/pldmd ${sysconfdir} ${prefix}/bin/pldmd-util"
