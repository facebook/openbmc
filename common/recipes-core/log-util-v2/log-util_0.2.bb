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

LOCAL_URI = " \
    file://Makefile \
    file://main.cpp \
    file://selformat.hpp \
    file://selformat.cpp \
    file://selstream.hpp \
    file://selstream.cpp \
    file://selexception.hpp \
    file://log-util.hpp \
    file://log-util.cpp \
    file://rsyslogd.hpp \
    file://rsyslogd.cpp \
    file://exclusion.hpp \
    file://tests/test_rsyslogd.cpp \
    file://tests/test_selformat.cpp \
    file://tests/test_selstream.cpp \
    file://tests/test_logutil.cpp \
    "

PROVIDES += "log-util-v2"
RPROVIDES:${PN} += "log-util-v2"

inherit ptest
do_compile_ptest() {
  make log-util-test
  cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
/usr/lib/log-util/ptest/log-util-test
EOF
}

do_install_ptest() {
  install -D -m 755 log-util-test ${D}${libdir}/log-util/ptest/log-util-test
}

do_install() {
  install -D -m 755 log-util ${D}${prefix}/local/bin/log-util
}

DEPENDS += "libpal cli11 nlohmann-json gtest gmock"
RDEPENDS:${PN} += "libpal"
RDEPENDS:${PN}-ptest += "libpal"

FILES:${PN} = "${prefix}/local/bin/log-util"
FILES:${PN}-ptest = "${libdir}/log-util/ptest"
