# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NAME Utility"
DESCRIPTION = "Util for showing valid FRU name on the platform"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://name-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://Makefile \
           file://name-util.cpp \
          "

S = "${WORKDIR}"
binfiles = "name-util"

LDFLAGS += "-lpal -ljansson"

pkgdir = "name-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 name-util ${dst}/name-util
  ln -snf ../fbpackages/${pkgdir}/name-util ${bin}/name-util
}

DEPENDS += "libpal jansson"
RDEPENDS_${PN} += "libpal jansson"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/name-util ${prefix}/local/bin"
