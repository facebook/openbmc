# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Firmware Utility"
DESCRIPTION = "Utility to upgrade firmware and to display firmware version of various components in the system"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fw-util.cpp;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

inherit native

SRC_URI =+ "file://Makefile \
           file://fw-util.h \
           file://fw-util.cpp \
           file://fw-util-test.cpp \
           file://bmc.cpp \
           file://bmc.h \
           file://bmc-test.cpp \
           file://check_image.cpp \
           file://image_parts.json \
           file://system_mock.cpp \
           file://extlib.cpp \
           file://extlib.h \
          "

S = "${WORKDIR}"

CXXFLAGS += "-D__TEST__ "
LDFLAGS =+ " -lpthread -ljansson -lfdt -lcrypto -lz -ldl -lgtest -lgmock "
DEPENDS += "jansson dtc-native zlib openssl-native dtc-native gtest-native "
RDEPENDS_${PN} += "jansson zlib openssl-native gtest-native "

do_install() {
  install -d ${D}${bindir}
  install -m 0755 fw-util ${D}${bindir}/fw-util
  install -d ${D}${sysconfdir}
  install -m 0644 ${WORKDIR}/image_parts.json ${D}${sysconfdir}/image_parts.json
}

FILES_${PN} = "${prefix}/bin"
FILES_${PN} += "${sysconfdir}/image_parts.json"
