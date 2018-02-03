# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "IPMI Utility"
DESCRIPTION = "Util for interacting with IPMI stack"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmi-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://ipmi-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "ipmi-util \
           "

pkgdir = "ipmi-util"

DEPENDS = " libipmi libpal "

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

FILES_${PN} = "${FBPACKAGEDIR}/ipmi-util ${prefix}/local/bin"

RDEPENDS_${PN} = "libipmi libpal"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
