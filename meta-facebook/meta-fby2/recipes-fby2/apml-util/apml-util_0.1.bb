# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "APML Utility"
DESCRIPTION = "Util for communicating to SOC PSP"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://apml-util.c;beginline=4;endline=16;md5=bd1371ec5d5f1f2a71a5549cb6bcbf2e"

LOCAL_URI = " \
    file://apml-util.c \
    file://Makefile \
    "

binfiles = "apml-util \
           "

pkgdir = "apml-util"

DEPENDS = " libbic libfby2-common"
RDEPENDS:${PN} += "libbic libfby2-common"

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

FILES:${PN} = "${FBPACKAGEDIR}/apml-util ${prefix}/local/bin"
