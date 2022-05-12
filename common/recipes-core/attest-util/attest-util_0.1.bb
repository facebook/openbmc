# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "MCTP Attestation Tool"
DESCRIPTION = "An attestation tool for different message types over MCTP"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://main.cpp;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig
inherit ptest-meson
LOCAL_URI = " \
    file://main.cpp \
    file://meson.build \
    file://message_types/spdm.cpp \
    file://message_types/spdm.hpp \
    file://tests/test_util.cpp \
    file://utils.cpp \
    file://utils.hpp \
    "

DEPENDS += " cli11  nlohmann-json gtest libpal libobmc-mctp "
RDEPENDS:${PN} += " libpal libobmc-mctp "
RDEPENDS:${PN}-ptest += " libpal libobmc-mctp "
FILES:${PN} = "${prefix}/local/bin/attest-util"

