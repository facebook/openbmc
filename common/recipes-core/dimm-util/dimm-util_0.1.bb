# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "DIMM Utility"
DESCRIPTION = "Util to retrieve DIMM information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://dimm-util.cpp;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"

LOCAL_URI = " \
    file://Makefile \
    file://dimm-util.cpp \
    file://dimm-util.h \
    file://dimm-vendor.cpp \
    file://dimm-capacity.cpp \
    "

binfiles = "dimm-util"

CXXFLAGS += " -lpal -lkv"
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

DEPENDS += "libpal libkv jansson"
RDEPENDS:${PN} += "libpal libkv jansson"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/dimm-util ${prefix}/local/bin"
