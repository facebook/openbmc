# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Firmware Utility"
DESCRIPTION = "Utility to upgrade firmware and to display firmware version of various components in the system"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fw-util.cpp;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

SRC_URI =+ "file://Makefile \
           file://fw-util.h \
           file://fw-util.cpp \
           file://system.cpp \
           file://server.h \
           file://server.cpp \
           file://nic.cpp \
           file://bic_fw.cpp \
           file://bic_fw.h \
           file://bmc.cpp \
           file://bmc.h \
           file://check_image.cpp \
           file://image_parts.json \
           file://platform.cpp \
          "

S = "${WORKDIR}"

#The definition for the project
CXXFLAGS += " -DBIC_SUPPORT "
DEPENDS += "libipmi libipmb libbic libocpdbg-lcd"
RDEPENDS_${PN} += "libipmi libipmb libbic libocpdbg-lcd"
LDFLAGS += " -lipmi -lipmb -lbic -locpdbg-lcd "

LDFLAGS =+ " -lpthread -ljansson -lfdt -lcrypto -lz -lpal -lvbs -ldl "
DEPENDS += "jansson libpal dtc zlib openssl libvbs "
RDEPENDS_${PN} += "jansson libpal zlib openssl libvbs "

do_install() {
  install -d ${D}${bindir}
  install -m 0755 fw-util ${D}${bindir}/fw-util
  install -d ${D}${sysconfdir}
  install -m 0644 ${WORKDIR}/image_parts.json ${D}${sysconfdir}/image_parts.json
}

FILES_${PN} = "${prefix}/bin"
FILES_${PN} += "${sysconfdir}/image_parts.json"
