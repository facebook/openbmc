# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "fruid-util-v2"
DESCRIPTION = "Utility for reading fruid information from fru-svc"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fruid-util-v2.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

LOCAL_URI = " \
    file://Makefile \
    file://fruid-util-v2.cpp \
    "

binfiles = "fruid-util-v2"

LDFLAGS =+ "-lglib-2.0 -lgio-2.0 -lgobject-2.0"
DEPENDS =+ "glib-2.0"

pkgdir = "fruid-util-v2"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

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

FILES:${PN} = "${FBPACKAGEDIR}/fruid-util-v2 ${prefix}/local/bin"
