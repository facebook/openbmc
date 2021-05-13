# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "GUID Utility"
DESCRIPTION = "Util for reading or updating GUID"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://guid-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://guid-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "guid-util \
           "

pkgdir = "guid-util"

DEPENDS = "libpal"
RDEPENDS_${PN} = "libpal"

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

FILES_${PN} = "${FBPACKAGEDIR}/guid-util ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
