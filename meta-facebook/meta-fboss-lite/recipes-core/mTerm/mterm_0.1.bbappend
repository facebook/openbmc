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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://mTerm_server.service \
            file://mTerm/run \
            file://sol.sh \
           "

S = "${WORKDIR}"

MTERM_CONSOLES ?= "115200;ttyS1"

MTERM_SYSTEMD_SERVICES = "mTerm_server.service"

do_install:append() {
    install -d ${D}/usr/local/bin
    install -m 0755 ${S}/sol.sh ${D}/usr/local/bin/

    #
    # fixup serial devices and baudrate
    #
    baudrate=`echo "${MTERM_CONSOLES}" | sed 's/\;.*//'`
    term=`echo "${MTERM_CONSOLES}" | sed 's/.*;//'`

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false',  d)}; then
        for svc in ${MTERM_SYSTEMD_SERVICES}; do
            sed -i -e "s/\@BAUDRATE\@/$baudrate/g" ${D}${systemd_system_unitdir}/${svc}
            sed -i -e "s/\@TERM\@/$term/g" ${D}${systemd_system_unitdir}/${svc}
        done
    else
        for svc in ${MTERM_SERVICES}; do
            sed -i -e "s/\@BAUDRATE\@/$baudrate/g" ${D}${sysconfdir}/sv/${svc}/run
            sed -i -e "s/\@TERM\@/$term/g" ${D}${sysconfdir}/sv/${svc}/run
        done
    fi

    sed -i -e "s/\@BAUDRATE\@/$baudrate/g" ${D}/usr/local/bin/sol.sh
    sed -i -e "s/\@TERM\@/$term/g" ${D}/usr/local/bin/sol.sh
}
