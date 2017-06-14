# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "IPMB Utility"
DESCRIPTION = "Util for communicating with an IPMB endpoint"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmb-util.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://ipmb-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "ipmb-util \
           "

pkgdir = "ipmb-util"

DEPENDS = " libipmb libipmi "

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

FILES_${PN} = "${FBPACKAGEDIR}/ipmb-util ${prefix}/local/bin"

RDEPENDS_${PN} = "libipmi libipmb bash"
