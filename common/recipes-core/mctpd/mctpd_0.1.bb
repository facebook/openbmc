SUMMARY = "mctp daemon"
DESCRIPTION = "The mctp daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mctpd.cpp;beginline=4;endline=16;md5=1b9fbafb77a4e5fffaa2d54854ecda68"

LOCAL_URI = " \
    file://Makefile \
    file://mctpd.cpp \
    file://mctpd.hpp \
    file://mctpd_plat.hpp \
    "

LDFLAGS += "-lmctp_intel -lipmi"

DEPENDS += "libmctp-intel libipmi cli11"
DEPENDS += "update-rc.d-native"
RDEPENDS_${PN} = "libmctp-intel libipmi"

binfiles = "mctpd"

pkgdir = "mctpd"
