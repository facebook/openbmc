# Copyright 2016-present Facebook. All Rights Reserved.
SUMMARY = "Fan Utility"
DESCRIPTION = "Util for controlling the Fan Speed"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fan-util.cpp;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://fan-util.cpp \
    "

binfiles = "fan-util"

DEPENDS =+ " libpal cli11 "

pkgdir = "fan-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 fan-util ${dst}/fan-util
  ln -snf ../fbpackages/${pkgdir}/fan-util ${bin}/fan-util
}

RDEPENDS:${PN} += "libpal"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/fan-util ${prefix}/local/bin"
