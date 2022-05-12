# Copyright 2017-present Facebook. All Rights Reserved.
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

inherit distutils3 ptest

SUMMARY = "Check U-Boot partitions"
DESCRIPTION = "Read, verify, and potentially modify U-Boot (firmware \
environment, legacy) partitions within image files or Memory Technology \
Devices."
SECTION = "base"
PR = "r3"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://partition.py;beginline=3;endline=16;\
md5=0b1ee7d6f844d472fa306b2fee2167e0"

LOCAL_URI = " \
    file://check_image.py \
    file://improve_system.py \
    file://partition.py \
    file://setup.py \
    file://system.py \
    file://test_improve_system.py \
    file://test_partition.py \
    file://test_system.py \
    file://test_virtualcat.py \
    file://virtualcat.py \
    file://bmc_pusher \
    "

do_lint() {
  flake8 *.py
  mypy *.py
  shellcheck bmc_pusher
}

do_compile_ptest() {
  cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
python -m unittest discover /usr/local/fbpackages/pypartition
EOF
}

do_install:class-target() {
  dst="${D}/usr/local/fbpackages/pypartition"
  install -d $dst
  for f in ${S}/*.py; do
    n=$(basename $f)
    install -m 755 "$f" ${dst}/$n
  done
}

# We don't have dependencies on flake8/mypy/shellcheck so we can't do this
# right now otherwise we get something like:
# pypartition-native/1.0-r3/temp/run.do_lint.1070986:
#       line 114: flake8: command not found
#
# addtask lint after do_compile before do_install

BBCLASSEXTEND += "native nativesdk"
FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/pypartition"
FILES:${PN}-ptest = "${libdir}/pypartition/ptest/run-ptest"
