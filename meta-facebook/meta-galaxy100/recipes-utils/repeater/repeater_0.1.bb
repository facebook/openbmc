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

SUMMARY = "Repeater Utilities"
DESCRIPTION = "Util for Repeater"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://repeater_card.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

SRC_URI = "file://repeater_card.sh \
		file://repeater_scm.sh \
          "
S = "${WORKDIR}"

do_install() {
	#init
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 repeater_card.sh ${D}${sysconfdir}/init.d/repeater_card.sh
	update-rc.d -r ${D} repeater_card.sh start 110 2 3 4 5 .
	install -m 0755 repeater_scm.sh ${D}${sysconfdir}/init.d/repeater_scm.sh
	update-rc.d -r ${D} repeater_scm.sh start 110 2 3 4 5 .
}

FILES_${PN} = "${sysconfdir}"
