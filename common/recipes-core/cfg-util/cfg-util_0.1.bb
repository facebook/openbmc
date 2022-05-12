# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Configuration Utility"
DESCRIPTION = "Utility for reading or writing configuration"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://cfg-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://cfg-util.cpp \
    "

LDFLAGS =+ " -lpal -lkv -lmisc-utils "
CXXFLAGS += '-Wall -Werror'

DEPENDS =+ " libpal libkv libmisc-utils "

binfiles = "cfg-util"

pkgdir = "cfg-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 cfg-util ${dst}/cfg-util
  ln -snf ../fbpackages/${pkgdir}/cfg-util ${bin}/cfg-util
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/cfg-util ${prefix}/local/bin"

RDEPENDS:${PN} = "libpal libmisc-utils "
