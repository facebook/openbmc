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
SUMMARY = "Galaxy100 memory test Utilities"
DESCRIPTION = "Util for Galaxy100 memory"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=0636e73ff0215e8d672dc4c32c317bb3"

LOCAL_URI = " \
    file://memtester.c \
    file://tests.c \
    file://memtester.h \
    file://types.h \
    file://sizes.h \
    file://tests.h \
    file://Makefile \
    file://COPYING \
    "

do_install() {
	install -d ${D}${bindir}
    install -m 0755 memtester ${D}${bindir}/memtester
}

FILES:${PN} = "${bindir}"
