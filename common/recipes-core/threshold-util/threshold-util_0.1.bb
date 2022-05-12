# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Threshold Utility"
DESCRIPTION = "Util for modify sensor threshold temporarily"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://threshold-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://threshold-util.c \
    "

binfiles = "threshold-util"

DEPENDS =+ " libsdr libpal "
RDEPENDS:${PN} =+ "libsdr libpal "

pkgdir = "threshold-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 threshold-util ${dst}/threshold-util
  ln -snf ../fbpackages/${pkgdir}/threshold-util ${bin}/threshold-util
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/threshold-util ${prefix}/local/bin"
