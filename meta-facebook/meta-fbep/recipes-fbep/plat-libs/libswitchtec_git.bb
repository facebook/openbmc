# Copyright 2017-present Facebook. All Rights Reserved.
HOMEPAGE = "https://github.com/Microsemi/switchtec-user"
SUMMARY = "Switchtec Library"
DESCRIPTION = "Library for communicating with Mircosemi PCIe switch"
SECTION = "libs"
LICENSE = "MIT"
PV = "0.9.5"
PR = "rc1"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=3d6b07c89629cff2990d2e8e1f4c2382"

SRC_URI = "git://github.com/Microsemi/switchtec-user.git;branch=devel;protocol=git"
SRCREV = "8357e463f85682e9798d52ba11b78b5e0f67fdca"

S = "${WORKDIR}/git"

do_configure() {
  ./configure --host=${HOST_SYS}
}

do_install() {
  install -d ${D}${libdir}
  install libswitchtec.so ${D}${libdir}/libswitchtec.so
  install -d ${D}${includedir}/switchtec
  install -m 0644 inc/switchtec/*.h ${D}${includedir}/switchtec/
  install -d ${D}${bindir}
  install -m 755 switchtec ${D}${bindir}/switchtec
}

FILES_${PN} = "${libdir}/libswitchtec.so ${bindir}/switchtec"
FILES_${PN}-dev = "${includedir}/switchtec"
