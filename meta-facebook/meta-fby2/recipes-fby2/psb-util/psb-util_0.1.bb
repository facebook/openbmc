# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "PSB Utility"
DESCRIPTION = "Util for reading PSB config"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://psb-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://psb-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "psb-util \
           "

pkgdir = "psb-util"

DEPENDS = " libpal libkv"
RDEPENDS_${PN} += " libpal libkv"

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

FILES_${PN} = "${FBPACKAGEDIR}/psb-util ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
