# Copyright 2014-present Facebook. All Rights Reserved.
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
SUMMARY = "Node Manager Library"
DESCRIPTION = "library for Node Manager"
SECTION = "base"
PR = "r1"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://nm.c;beginline=8;endline=20;md5=da58bc07a669261cb7ccf2838db71124"


inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://nm.c \
    file://nm.h \
    "
DEPENDS =+ "libipmi libipmb"
