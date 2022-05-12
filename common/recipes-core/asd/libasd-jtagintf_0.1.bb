# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "JTAG Interface library"
DESCRIPTION = "Library which helps interfacing with the JTAG hardware"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://SoftwareJTAGHandler.c;beginline=5;endline=26;md5=4d3dd6a70786d475883b1542b0898219"

SRC_URI = "file://interface/SoftwareJTAGHandler.c \
           file://interface/SoftwareJTAGHandler.h \
           file://interface/pin_interface.c \
           file://interface/pin_interface.h \
           file://interface/mem_helper.c \
           file://interface/mem_helper.h \
           file://interface/Makefile \
           "

S = "${WORKDIR}/interface"

do_install() {
  install -d ${D}${includedir}/asd
  install -m 0644 SoftwareJTAGHandler.h ${D}${includedir}/asd/SoftwareJTAGHandler.h
  install -m 0644 pin_interface.h ${D}${includedir}/asd/pin_interface.h

  install -d ${D}${libdir}
  install -m 0644 libasd-jtagintf.so ${D}${libdir}/libasd-jtagintf.so
}

FILES:${PN} = "${libdir}/libasd-jtagintf.so"
FILES:${PN}-dev = "${includedir}/asd"
