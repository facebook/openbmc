# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "IPC Helper Library"
DESCRIPTION = "library to provide NC-SI functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ncsi.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://ncsi.c \
    file://ncsi.h \
    file://aen.c \
    file://aen.h \
    file://ncsi-brcm.c \
    file://ncsi-brcm.h \
    file://ncsi-nvidia.c \
    file://ncsi-nvidia.h \
    "

DEPENDS =+ " libkv"
CFLAGS += "  -lkv"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libncsi.so ${D}${libdir}/libncsi.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ncsi.h ${D}${includedir}/openbmc/ncsi.h
    install -m 0644 aen.h ${D}${includedir}/openbmc/aen.h
    install -m 0644 ncsi-brcm.h ${D}${includedir}/openbmc/ncsi-brcm.h
    install -m 0644 ncsi-nvidia.h ${D}${includedir}/openbmc/ncsi-nvidia.h
}

DEPENDS += "libkv"
RDEPENDS:${PN} = "libkv"

FILES:${PN} = "${libdir}/libncsi.so"
FILES:${PN}-dev = "${includedir}/openbmc/*"
