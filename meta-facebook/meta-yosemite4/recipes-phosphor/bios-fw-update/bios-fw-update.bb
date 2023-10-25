SUMMARY = "FB BIOS UPDATE TOOL"
DESCRIPTION = "tool for update bios"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"

LICENSE="CLOSED"

S = "${WORKDIR}"

inherit meson pkgconfig

SRC_URI = "file://bios-update.hpp \
           file://bios-update.cpp \
           file://bios-usb-update.hpp \
           file://bios-usb-update.cpp \
           file://meson.build \
          "

LDFLAGS = "-lusb-1.0"
DEPENDS += "systemd libusb1 cli11 sdbusplus boost"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 bios-fw-update ${D}/${sbindir}/
}