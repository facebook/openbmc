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
#

SUMMARY = "CPLD Update Utilities"
DESCRIPTION = "Util for CPLD update using JTAG master Hardware Mode"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

SRC_URI = "file://lattice_cpld.c \
           file://lattice_cpld.h \
           file://main.c \
           file://jtag_interface.c \
           file://jtag_interface.h \
           file://jtag_common.c \
           file://jtag_common.h \
           file://jedecfile.c \
           file://jedecfile.h \
           file://Makefile \
           file://COPYING \
           file://linux/jtag.h \
          "

S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 cpldprog ${D}${bindir}/cpldprog
}

DEPENDS += ""
