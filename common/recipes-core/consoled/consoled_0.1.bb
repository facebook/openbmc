# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Serial Buffer"
DESCRIPTION = "Daemon for serial buffering"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://consoled.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://Makefile \
           file://consoled.c \
          "
S = "${WORKDIR}"

LDFLAGS =+ " -lpal "

DEPENDS =+ " libpal "
RDEPENDS_${PN} =+ "libpal"

binfiles = "consoled"

pkgdir = "consoled"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 consoled ${dst}/consoled
  ln -snf ../fbpackages/${pkgdir}/consoled ${bin}/consoled
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/consoled ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
