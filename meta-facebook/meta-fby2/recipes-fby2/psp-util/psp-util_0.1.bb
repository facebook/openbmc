# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "APML Utility"
DESCRIPTION = "Util for communicating to SOC PSP"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://psp-util.c;beginline=4;endline=16;md5=eb69e90379b8d03d819049f47e47611f"

SRC_URI = "file://psp-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "psp-util \
           "

pkgdir = "psp-util"

DEPENDS = " libbic "
RDEPENDS_${PN} += "libbic "

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

FILES_${PN} = "${FBPACKAGEDIR}/psp-util ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
