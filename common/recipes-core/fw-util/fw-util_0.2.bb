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
           file://system_intf.h \
           file://server.h \
           file://server.cpp \
           file://bmc.cpp \
           file://bmc.h \
           file://pfr_bmc.cpp \
           file://pfr_bmc.h \
           file://check_image.cpp \
           file://nic.h \
           file://nic.cpp \
           file://fscd.cpp \
           file://tpm.cpp \
           file://tpm.h \
           file://tpm2.cpp \
           file://tpm2.h \
           file://bic_fw.cpp \
           file://bic_fw.h \
           file://bic_me.cpp \
           file://bic_me.h \
           file://bic_bios.cpp \
           file://bic_bios.h \
           file://bic_cpld.cpp \
           file://bic_cpld.h \
           file://extlib.cpp \
           file://extlib.h \
           file://spiflash.cpp \
           file://spiflash.h \
           file://image_parts.json \
           file://scheduler.h \
           file://scheduler.cpp \
          "

S = "${WORKDIR}"

LDFLAGS =+ " -lpthread -lfdt -lcrypto -lz -lpal -lvbs -ldl -lgpio-ctrl -lkv -lobmc-i2c"
DEPENDS += " nlohmann-json libpal dtc zlib openssl libvbs libgpio-ctrl libkv libobmc-i2c"
RDEPENDS_${PN} += " libpal zlib openssl libvbs libgpio-ctrl libkv libobmc-i2c"

CXXFLAGS += "\
  ${@bb.utils.contains('MACHINE_FEATURES', 'tpm1', '-DCONFIG_TPM1', '', d)} \
  ${@bb.utils.contains('MACHINE_FEATURES', 'tpm2', '-DCONFIG_TPM2', '', d)} \
  "
do_install() {
  install -d ${D}${bindir}
  install -m 0755 fw-util ${D}${bindir}/fw-util
  install -d ${D}${sysconfdir}
  install -m 0644 ${WORKDIR}/image_parts.json ${D}${sysconfdir}/image_parts.json
}

FILES_${PN} = "${prefix}/bin"
FILES_${PN} += "${sysconfdir}/image_parts.json"
