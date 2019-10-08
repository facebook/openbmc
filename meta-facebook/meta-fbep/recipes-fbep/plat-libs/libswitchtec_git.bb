# Copyright 2017-present Facebook. All Rights Reserved.
HOMEPAGE = "https://github.com/Microsemi/switchtec-user"
SUMMARY = "Switchtec Library"
DESCRIPTION = "Library for communicating with Mircosemi PCIe switch"
SECTION = "libs"
LICENSE = "MIT"
PV = "1.3"
PR = "rc4"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=3d6b07c89629cff2990d2e8e1f4c2382"

SRC_URI = "git://github.com/Microsemi/switchtec-user.git;tag=v${PV}-${PR};protocol=git;nobranch=1"
# SRCREV = "cbcad45fe9b2d21c52ff13e810a8bad56f9d4f03"

S = "${WORKDIR}/git"

do_configure() {
  ./configure --host=${HOST_SYS}
}

do_install() {
  install -d ${D}${libdir}
  install libswitchtec.so ${D}${libdir}/libswitchtec.so
  lnr ${D}${libdir}/libswitchtec.so ${D}${libdir}/libswitchtec.so.1
  install -d ${D}${includedir}/switchtec
  install -m 0644 inc/switchtec/*.h ${D}${includedir}/switchtec/
  install -d ${D}${bindir}
  install -m 755 switchtec ${D}${bindir}/switchtec
}

FILES_${PN} = "${libdir}/libswitchtec.so ${libdir}/libswitchtec.so.1 ${bindir}/switchtec"
FILES_${PN}-dev = "${includedir}/switchtec"
