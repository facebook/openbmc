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
SUMMARY = "Log Utility"
DESCRIPTION = "Utility to parse and display logs."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://main.cpp;beginline=5;endline=18;md5=ff9a2ba58fa5b39d3d3dcb7c42e26496"

SRC_URI = "file://log-util"
S = "${WORKDIR}/log-util"

inherit meson pkgconfig
inherit ptest-meson
inherit legacy-packages

DEPENDS += "libpal cli11 nlohmann-json gtest gmock"

PROVIDES += "log-util-v2"
RPROVIDES:${PN} += "log-util-v2"
