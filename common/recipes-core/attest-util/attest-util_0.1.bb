# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "MCTP Attestation Tool"
DESCRIPTION = "An attestation tool for different message types over MCTP"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://main.cpp;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson
inherit ptest-meson

SRC_URI = "\
    file://main.cpp \
    file://meson.build \
    file://message_types/spdm.cpp \
    file://message_types/spdm.hpp \
    file://tests/test_util.cpp \
    file://utils.cpp \
    file://utils.hpp \
    "

S = "${WORKDIR}"

DEPENDS += " cli11  nlohmann-json gtest"
RDEPENDS_${PN} += ""

