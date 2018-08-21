# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Management Engine Utility"
DESCRIPTION = "Util for communicating to Qualcomm IMC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://imc-util.c;beginline=4;endline=16;md5=7c59cc81f1be0a56c1db0773f0ad24f6"

SRC_URI = "file://imc-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "imc-util \
           "

pkgdir = "imc-util"

DEPENDS = " libbic libpal libfby2-sensor "
RDEPENDS_${PN} += "libbic libpal libfby2-sensor libipmi libipmb"

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

FILES_${PN} = "${FBPACKAGEDIR}/imc-util ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
