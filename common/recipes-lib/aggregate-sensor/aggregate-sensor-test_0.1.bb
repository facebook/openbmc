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

SUMMARY = "Aggregate Sensor Unit Test"
DESCRIPTION = "Aggregate sensor unit test"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://aggregate-sensor-test.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit native

SRC_URI = "file://test/Makefile \
           file://test/aggregate-sensor-test.c \
           file://aggregate-sensor.c \
           file://aggregate-sensor.h \
           file://aggregate-sensor-internal.h \
           file://aggregate-sensor-json.c \
           file://math_expression.c \
           file://math_expression.h \
           file://test/test_null.json \
           file://test/test_lexp.json \
           file://test/test_lexp_sexp.json \
           file://test/test_clexp.json \
          "
S = "${WORKDIR}/test"

DEPENDS += " jansson libsdr-native libpal-native libkv-native cmock-native "
RDEPENDS_${PN} += "jansson "

do_install() {
  bin="${D}/usr/local/bin"
  install -d $bin
  install -m 755 aggregate-sensor-test ${bin}/aggregate-sensor-test
  for j in *.json; do
    install -m 0644 ${j} ${bin}/${j}
  done
}
FILES_${PN} = "${prefix}/local/bin"

