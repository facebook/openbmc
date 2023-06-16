#
SUMMARY = "Openbmc PLDM Middleware"
DESCRIPTION = "Middleware for Openbmc PLDM Library"

SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pldm.h;beginline=1;endline=1;md5=dcde81a56f7a67fc2c0d15658020f83a"

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
    file://fw_update.hpp \
    "

DEPENDS += "libpldm libipmi"
RDEPENDS:${PN} += "libpldm  libipmi"
LDFLAGS += "-lpldm -lipmi"

inherit meson pkgconfig
