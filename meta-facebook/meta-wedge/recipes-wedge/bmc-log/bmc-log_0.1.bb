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
#

SUMMARY = "Logging Utility"
DESCRIPTION = "Util for logging"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bmc-log.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://bmc-log.c \
	   file://bmc-log.h \
	   file://Makefile \
	   file://bmc-log-config \
	   file://bmc-log.sh \
          "

S = "${WORKDIR}"

do_install() {
	install -d ${D}${sbindir}
	install -m 755 bmc-log ${D}${sbindir}/bmc-log
	install -d ${D}${sysconfdir}/default
	install -m 755 bmc-log-config ${D}${sysconfdir}/default/bmc-log
	install -d ${D}${sysconfdir}/init.d
	install -m 755 bmc-log.sh ${D}${sysconfdir}/init.d/bmc-log.sh
	update-rc.d -r ${D} bmc-log.sh start 92 S .
}

FILES_${PN} = "${sbindir} ${sysconfdir} "

# Inhibit complaints about .debug directories

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
