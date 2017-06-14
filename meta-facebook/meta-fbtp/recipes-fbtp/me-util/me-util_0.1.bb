# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Management Engine Utility"
DESCRIPTION = "Util for communicating to Intel ME"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://me-util;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

SRC_URI = "file://me-util \
          "

S = "${WORKDIR}"

binfiles = "me-util \
           "

pkgdir = "me-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/me-util ${prefix}/local/bin"

RDEPENDS_${PN} = "bash ipmb-util "
