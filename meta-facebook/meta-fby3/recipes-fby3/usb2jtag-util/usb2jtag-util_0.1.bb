# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "USB2JTAG Utility"
DESCRIPTION = "USB2JTAG Utility"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://usb2jtag-util.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

inherit meson pkgconfig

LOCAL_URI = " \
    file://usb2jtag-util.c \
    file://meson.build \
    "

LDFLAGS += " -lpal "
DEPENDS += " libobmc-i2c libusb1 libftdi libmpsse libpal "
RDEPENDS:${PN} += " libftdi "

