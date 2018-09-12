# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "IPC Helper Library"
DESCRIPTION = "library to abstract away IPC from user (Underlying implementation could be socket or dbus)"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipc.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

SRC_URI = "file://Makefile \
           file://ipc.c \
           file://ipc.h \
          "

S = "${WORKDIR}"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libipc.so ${D}${libdir}/libipc.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ipc.h ${D}${includedir}/openbmc/ipc.h
}

FILES_${PN} = "${libdir}/libipc.so"
FILES_${PN}-dev = "${includedir}/openbmc/ipc.h"
