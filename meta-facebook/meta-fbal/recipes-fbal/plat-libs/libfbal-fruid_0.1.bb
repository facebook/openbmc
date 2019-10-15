# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "FBAL FRUID Library"
DESCRIPTION = "FBAL FRUID Library"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fbal_fruid.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://fbal_fruid \
          "

DEPENDS += "libipmi libipmb"
RDEPENDS_${PN} += "libipmb"
LDFLAGS += "-lipmb"

S = "${WORKDIR}/fbal_fruid"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libfbal-fruid.so ${D}${libdir}/libfbal-fruid.so

  install -d ${D}${includedir}/facebook
  install -m 0644 fbal_fruid.h ${D}${includedir}/facebook/fbal_fruid.h
}

FILES_${PN} = "${libdir}/libfbal-fruid.so"
FILES_${PN}-dev = "${includedir}/facebook/fbal_fruid.h"
