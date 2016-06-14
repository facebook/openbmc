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
SUMMARY = "Galaxy100 stress test Utilities"
DESCRIPTION = "Util for Galaxy100 stress test"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"


SRC_URI = "file://stress.c \
		file://Makefile \
		file://COPYING"

S = "${WORKDIR}"
do_install() {
	install -d ${D}${bindir}
	install -m 0755 stress ${D}${bindir}/stress
}

FILES_${PN} = "${bindir}"
