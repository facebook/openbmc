# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic.h;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"


SRC_URI = "file://bic \
          "

SOURCES = "bic_xfer.c bic_power.c bic_ipmi.c bic_fwupdate.c bic_cpld_altera_fwupdate.c bic_cpld_lattice_fwupdate.c bic_vr_fwupdate.c bic_bios_fwupdate.c bic_bios_usb_fwupdate.c"
HEADERS = "bic.h bic_xfer.h bic_power.h bic_ipmi.h bic_fwupdate.h bic_cpld_altera_fwupdate.h bic_cpld_lattice_fwupdate.h bic_vr_fwupdate.h bic_bios_fwupdate.h"

CFLAGS += " -Wall -Werror -fPIC "
LDFLAGS = "-lobmc-i2c -lipmb -lgpio-ctrl -lusb-1.0"

DEPENDS += "libipmi libipmb libobmc-i2c libgpio-ctrl libfby3-common libkv libusb1"
RDEPENDS_${PN} += "libobmc-i2c libgpio-ctrl libfby3-common"

S = "${WORKDIR}/bic"

do_compile() {
  make SOURCES="${SOURCES}" HEADERS="${HEADERS}"
}

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libbic.so ${D}${libdir}/libbic.so
    ln -s libbic.so ${D}${libdir}/libbic.so.0

    install -d ${D}${includedir}/facebook
    for f in ${HEADERS}; do
      install -m 0644 ${S}/$f ${D}${includedir}/facebook/$f
    done
}

FILES_${PN} = "${libdir}/libbic.so*"
FILES_${PN}-dev = "${includedir}/facebook"
