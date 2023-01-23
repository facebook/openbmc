#
SUMMARY = "Openbmc PLDM Middleware"
DESCRIPTION = "Middleware for Openbmc PLDM Library"

SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pldm.h;beginline=4;endline=16;md5=7893c791eafb70877f7798ca929f0e41"

LOCAL_URI = " \
    file://meson.build \
    file://pldm.h \
    file://pldm.cpp \
    file://base.hpp \
    file://base.cpp \
    file://platform.hpp \
    file://platform.cpp \
    file://oem.hpp \
    file://oem.cpp \
    file://handler.hpp \
    file://fw_update.cpp \
    file://fw_update.h \
    "

DEPENDS += "libpldm  libipmi obmc-libpldm"
RDEPENDS:${PN} += "libpldm  libipmi obmc-libpldm"
LDFLAGS += "-lpldm -lipmi -lobmc-pldm"

inherit meson pkgconfig
