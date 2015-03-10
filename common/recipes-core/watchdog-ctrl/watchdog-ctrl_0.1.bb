# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Watchdog control utilities."
DESCRIPTION = "The utilities to control system watchdog."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://watchdog_ctrl.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

SRC_URI = "file://watchdog_ctrl.sh \
          "

S = "${WORKDIR}"

binfiles = "watchdog_ctrl.sh"

pkgdir = "watchdog_ctrl"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/${f}
    ln -s ../fbpackages/${pkgdir}/${f} ${bin}/${f}
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/watchdog_ctrl ${prefix}/local/bin ${sysconfdir} "
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
