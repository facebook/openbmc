# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Firmware Utility"
DESCRIPTION = "Utility to upgrade firmware and to display firmware version of various components in the system"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fw-util.cpp;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

LOCAL_URI =+ " \
    file://Makefile \
    file://fw-util.h \
    file://fw-util.cpp \
    file://system.cpp \
    file://system_mock.cpp \
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
    file://vr.cpp \
    file://tests/bmc-test.cpp \
    file://tests/fw-util-test.cpp \
    file://tests/system_mock.h \
    file://tests/nic-test.cpp \
    "

inherit ptest
do_compile_ptest() {
  make -j 16 fw-util-test
  cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
/usr/lib/fw-util/ptest/fw-util-test
EOF
}

do_install_ptest() {
  install -D -m 755 fw-util-test ${D}${libdir}/fw-util/ptest/fw-util-test
}

LDFLAGS += "-lpthread -lfdt -lcrypto -lz -lpal -lvbs -ldl -lgpio-ctrl -lkv -lobmc-i2c -lmisc-utils"
DEPENDS += "nlohmann-json libpal dtc zlib openssl libvbs libgpio-ctrl libkv libobmc-i2c libmisc-utils"
RDEPENDS:${PN} += "libpal zlib openssl libvbs libgpio-ctrl libkv libobmc-i2c libmisc-utils"
RDEPENDS:${PN}-ptest += "${RDEPENDS:${PN}}"

CXXFLAGS:append = " -Wno-psabi"
CXXFLAGS:append:mf-tpm1 = " -DCONFIG_TPM1"
CXXFLAGS:append:mf-tpm2 = " -DCONFIG_TPM2"

do_install() {
  install -d ${D}${bindir}
  install -m 0755 fw-util ${D}${bindir}/fw-util
  install -d ${D}${sysconfdir}
  install -m 0644 ${S}/image_parts.json ${D}${sysconfdir}/image_parts.json
}

FILES:${PN} = "${prefix}/bin"
FILES:${PN} += "${sysconfdir}/image_parts.json"
FILES:${PN}-ptest = "${libdir}/fw-util/ptest"
