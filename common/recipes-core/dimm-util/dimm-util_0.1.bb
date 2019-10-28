# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "DIMM Utility"
DESCRIPTION = "Util to retrieve DIMM information"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://dimm-util.cpp;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"

SRC_URI = "file://Makefile \
           file://dimm-util.cpp \
           file://dimm-util.h \
           file://dimm-vendor.cpp \
          "

S = "${WORKDIR}"
binfiles = "dimm-util"

CXXFLAGS += " -lpal -lkv -lbic"
LDFLAGS = " -ljansson"
pkgdir = "dimm-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 dimm-util ${dst}/dimm-util
  ln -snf ../fbpackages/${pkgdir}/dimm-util ${bin}/dimm-util
}

DEPENDS += "libpal libkv libbic jansson"
RDEPENDS_${PN} += "libpal libkv libbic jansson"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/dimm-util ${prefix}/local/bin"
