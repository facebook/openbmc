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

SUMMARY = "Configurable fan controller"
DESCRIPTION = "Fan controller"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fscd.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

LOCAL_URI = " \
    file://fscd.py \
    file://fsc_bmcmachine.py \
    file://fsc_board.py \
    file://fsc_common_var.py \
    file://fsc_control.py \
    file://fsc_expr.py \
    file://fsc_parser.py \
    file://fsc_profile.py \
    file://fsc_sensor.py \
    file://fsc_util.py \
    file://fsc_zone.py \
    file://setup.py \
    file://run-fscd.sh \
    file://fscd.service \
    file://fscd_test \
    "

inherit setuptools3
inherit systemd
inherit ptest
DEPENDS += "update-rc.d-native libkv libwatchdog"
RDEPENDS:${PN} += "python3-syslog python3-ply libkv libwatchdog"

FSC_BIN_FILES = ""

FSC_CONFIG = ""

FSC_ZONE_CONFIG = ""

FSC_INIT_FILE = ""

FSC_SV_FILE = "run-fscd.sh"

do_work_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/fscd
  if [ ! -z ${FSC_INIT_FILE} ]; then
    install -m 755 ${FSC_INIT_FILE} ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} ${FSC_INIT_FILE} start 91 5 .
  fi
  if [ ! -z ${FSC_SV_FILE} ]; then
    install -m 755 ${FSC_SV_FILE} ${D}${sysconfdir}/sv/fscd/run
  fi
}

do_work_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 fscd.service ${D}${systemd_system_unitdir}
}

do_install:append() {
    bin="${D}/usr/local/bin"
    install -d $bin
    install -d ${D}${sysconfdir}
    install -d ${D}${sysconfdir}/fsc
    for f in ${FSC_CONFIG}; do
        install -m 644 $f ${D}${sysconfdir}/$f
    done

    for f in ${FSC_ZONE_CONFIG}; do
        install -m 644 $f ${D}${sysconfdir}/fsc/$f
    done

    for f in ${FSC_BIN_FILES}; do
        install -m 755 $f ${bin}/$f
    done

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        do_work_systemd
    else
        do_work_sysv
    fi
}

do_compile_ptest() {
  cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
set -e
cd /usr/lib/fscd/ptest
if [ ! -d /var/volatile/log ]; then
  mkdir /var/volatile/log
fi
PYTHONPATH=/usr/bin python3 ./fsc_tester.py
EOF
}

do_install_ptest() {
  install -d ${D}${libdir}/fscd
  install -d ${D}${libdir}/fscd/ptest
  cp -r ${S}/fscd_test/* ${D}${libdir}/fscd/ptest/
}


FILES:${PN} += "${prefix}/local/bin ${sysconfdir} "

SYSTEMD_SERVICE:${PN} = "fscd.service"
