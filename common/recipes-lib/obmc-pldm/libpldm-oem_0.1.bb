#
SUMMARY = "Openbmc PLDM Middleware"
DESCRIPTION = "Middleware for Openbmc PLDM Library"

SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pldm.h;beginline=4;endline=16;md5=7daca318951174622e8e12a585f4e4a0"

LOCAL_URI = " \
    file://meson.build \
    file://plat/meson.build \
    file://pldm.h \
    file://pldm.cpp \
    file://base.hpp \
    file://base.cpp \
    file://platform.hpp \
    file://platform.cpp \
    file://platform_sensor.cpp \
    file://oem.hpp \
    file://oem.cpp \
    file://oem_pldm.cpp \
    file://oem_pldm.hpp \
    file://handler.hpp \
    file://fw_update.cpp \
    file://fw_update.h \
    "

DEPENDS += "libpldm libipmi"
RDEPENDS:${PN} += "libpldm  libipmi"
LDFLAGS += "-lpldm -lipmi"

inherit meson pkgconfig
