# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Netlink Wrapper Library"
DESCRIPTION = "library to provide Netlink wrapper functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://nl-wrapper.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://nl-wrapper.c \
    file://nl-wrapper.h \
    "

CFLAGS += "-I${STAGING_INCDIR}/libnl3"
LDFLAGS += "-lnl-3 -lnl-cli-3 -lnl-genl-3 -lnl-nf-3"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libnl-wrapper.so ${D}${libdir}/libnl-wrapper.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 nl-wrapper.h ${D}${includedir}/openbmc/nl-wrapper.h
}

DEPENDS += "libncsi libnl"
RDEPENDS:${PN} += "libnl"

FILES:${PN} = "${libdir}/libnl-wrapper.so"
FILES:${PN}-dev = "${includedir}/openbmc/*"
