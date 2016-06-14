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

SUMMARY = "Retimer Utilities"
DESCRIPTION = "Util for Retimer"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://retimer_card.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

SRC_URI = "file://retimer_card.sh \
		file://retimer_scm.sh \
          "
S = "${WORKDIR}"

do_install() {
	#init
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 retimer_card.sh ${D}${sysconfdir}/init.d/retimer_card.sh
	update-rc.d -r ${D} retimer_card.sh start 110 2 3 4 5 .
	install -m 0755 retimer_scm.sh ${D}${sysconfdir}/init.d/retimer_scm.sh
	update-rc.d -r ${D} retimer_scm.sh start 110 2 3 4 5 .
}

FILES_${PN} = "${sysconfdir}"
