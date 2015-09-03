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

SUMMARY = "IPMI Daemon"
DESCRIPTION = "Daemon to handle IPMI Messages."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmid.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://ipmid.c \
           file://timestamp.c \
           file://timestamp.h \
           file://sel.c \
           file://sel.h \
           file://sdr.c \
           file://sdr.h \
           file://sensor.h \
           file://fruid.h \
          "

DEPENDS += " libpal "

binfiles = "ipmid"

pkgdir = "ipmid"
