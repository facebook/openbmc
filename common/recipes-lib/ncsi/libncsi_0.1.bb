# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "IPC Helper Library"
DESCRIPTION = "library to provide NC-SI functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ncsi.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://ncsi.c \
           file://ncsi.h \
           file://aen.c \
           file://aen.h \
          "

S = "${WORKDIR}"

DEPENDS =+ " libkv"
CFLAGS += "  -lkv"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libncsi.so ${D}${libdir}/libncsi.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ncsi.h ${D}${includedir}/openbmc/ncsi.h
    install -m 0644 aen.h ${D}${includedir}/openbmc/aen.h
}

DEPENDS += "libkv"
RDEPENDS_${PN} = "libkv"

FILES_${PN} = "${libdir}/libncsi.so"
FILES_${PN}-dev = "${includedir}/openbmc/*"
