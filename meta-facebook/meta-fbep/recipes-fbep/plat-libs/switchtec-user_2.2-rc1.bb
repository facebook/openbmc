# Copyright 2017-present Facebook. All Rights Reserved.
HOMEPAGE = "https://github.com/Microsemi/switchtec-user"
SUMMARY = "Switchtec Library"
DESCRIPTION = "Library for communicating with Mircosemi PCIe switch"
SECTION = "libs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3d6b07c89629cff2990d2e8e1f4c2382"

SRC_URI = "https://github.com/Microsemi/switchtec-user/archive/v${PV}.tar.gz"
SRC_URI[md5sum] = "cb2228cafb8d9ee415d05f216574078c"
SRC_URI[sha256sum] = "88d3d8994de766798c9be61fca64268f5f0cf4c030ab1dca878f5239072c2a9c"

S = "${WORKDIR}/switchtec-user-${PV}"

DEPENDS += " openssl"
RDEPENDS_${PN} += " openssl"

do_configure() {
  ./configure --host=${HOST_SYS}
}

do_install() {
  install -d ${D}${libdir}
  install libswitchtec.so ${D}${libdir}/libswitchtec.so
  lnr ${D}${libdir}/libswitchtec.so ${D}${libdir}/libswitchtec.so.2
  install -d ${D}${includedir}/switchtec
  install -m 0644 inc/switchtec/*.h ${D}${includedir}/switchtec/
  install -d ${D}${bindir}
  install -m 755 switchtec ${D}${bindir}/switchtec
}

FILES_${PN} = "${libdir}/libswitchtec.so ${libdir}/libswitchtec.so.2 ${bindir}/switchtec"
FILES_${PN}-dev = "${includedir}/switchtec"
