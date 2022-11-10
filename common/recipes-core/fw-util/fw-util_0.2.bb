# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Firmware Utility"
DESCRIPTION = "Utility to upgrade firmware and to display firmware version of various components in the system"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fw-util.cpp;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

LOCAL_URI =+ " \
    file://meson.build \
    file://meson_options.txt \
    file://plat/meson.build \
    file://bic_bios.cpp \
    file://bic_bios.h \
    file://bic_cpld.cpp \
    file://bic_cpld.h \
    file://bic_fw.cpp \
    file://bic_fw.h \
    file://bic_me.cpp \
    file://bic_me.h \
    file://bmc.cpp \
    file://bmc.h \
    file://check_image.cpp \
    file://extlib.cpp \
    file://extlib.h \
    file://fscd.cpp \
    file://fw-util.cpp \
    file://fw-util.h \
    file://image_parts.json \
    file://nic.cpp \
    file://nic.h \
    file://pfr_bmc.cpp \
    file://pfr_bmc.h \
    file://scheduler.cpp \
    file://scheduler.h \
    file://server.cpp \
    file://server.h \
    file://spiflash.cpp \
    file://spiflash.h \
    file://system.cpp \
    file://system_intf.h \
    file://system_mock.cpp \
    file://tests/bmc-test.cpp \
    file://tests/fw-util-test.cpp \
    file://tests/nic-test.cpp \
    file://tests/system_mock.h \
    file://tpm.cpp \
    file://tpm.h \
    file://tpm2.cpp \
    file://tpm2.h \
    file://vr.cpp \
    "

inherit meson pkgconfig
inherit ptest-meson

PACKAGECONFIG ??= "bic fscd"
PACKAGECONFIG[bic] = "-Dbic=true,-Dbic=false"
PACKAGECONFIG[fscd] = "-Dfscd=true,-Dfscd=false"

DEPENDS += " \
    dtc \
    libgpio-ctrl \
    libkv \
    libmisc-utils \
    libobmc-i2c \
    libpal \
    libvbs \
    nlohmann-json \
    openssl \
    zlib \
    "
RDEPENDS:${PN} += "at"

# Since many systems have their own additions to RDEPENDS, due to unversioned
# libraries being compiled with custom makefiles, add all their RDEPENDS into
# the ptest as well to ensure no library dependencies are missing.
RDEPENDS:${PN}-ptest += "${RDEPENDS:${PN}}"

CXXFLAGS:append = " -Wno-psabi"
CXXFLAGS:append:mf-tpm1 = " -DCONFIG_TPM1"
CXXFLAGS:append:mf-tpm2 = " -DCONFIG_TPM2"

do_install:append() {
  install -d ${D}${sysconfdir}
  install -m 0644 ${S}/image_parts.json ${D}${sysconfdir}/image_parts.json
}
