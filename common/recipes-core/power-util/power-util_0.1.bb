# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Power Utility"
DESCRIPTION = "Utility for Power Policy and Management"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://power-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://power-util.c \
    "

LDFLAGS =+ " -lpal "

DEPENDS =+ " libpal "
RDEPENDS:${PN} =+ "libpal"

binfiles = "power-util"

pkgdir = "power-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 power-util ${dst}/power-util
  ln -snf ../fbpackages/${pkgdir}/power-util ${bin}/power-util
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/power-util ${prefix}/local/bin"
