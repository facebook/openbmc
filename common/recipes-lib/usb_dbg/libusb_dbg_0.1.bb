# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing common features for the LCD USB debug card"
DESCRIPTION = "Library providing common features for the LCD USB debug card"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://usb_dbg.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://usb_dbg.c \
           file://usb_dbg.h \
          "

S = "${WORKDIR}"

DEPENDS = " libipmb "

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libusb_dbg.so ${D}${libdir}/libusb_dbg.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 usb_dbg.h ${D}${includedir}/openbmc/usb_dbg.h
}

FILES_${PN} = "${libdir}/libusb_dbg.so"
FILES_${PN}-dev = "${includedir}/openbmc/usb_dbg.h"

PROVIDES = "libusb_dbg"
