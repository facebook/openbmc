# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "PECI Utility"
DESCRIPTION = "Util for interacting with PECI driver"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://peci-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://peci-util.c \
    file://Makefile \
    "

binfiles = "peci-util \
           "

pkgdir = "peci-util"

DEPENDS = "update-rc.d-native libpal"
RDEPENDS:${PN} = "libpal bash"

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

FILES:${PN} = "${FBPACKAGEDIR}/peci-util ${prefix}/local/bin"
