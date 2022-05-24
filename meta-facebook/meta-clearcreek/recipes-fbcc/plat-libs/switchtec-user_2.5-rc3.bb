# Copyright 2017-present Facebook. All Rights Reserved.
HOMEPAGE = "https://github.com/Microsemi/switchtec-user"
SUMMARY = "Switchtec Library"
DESCRIPTION = "Library for communicating with Mircosemi PCIe switch"
SECTION = "libs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3d6b07c89629cff2990d2e8e1f4c2382"

SRC_URI = "https://github.com/Microsemi/switchtec-user/archive/v${PV}.tar.gz"
SRC_URI[md5sum] = "a0748c19552a082342fb665db20e00a0"
SRC_URI[sha256sum] = "cabce7e83c3470546285d275a2586d95a9c74463fa6b498e89a30423713786d2"

S = "${WORKDIR}/switchtec-user-${PV}"

DEPENDS += " openssl"
RDEPENDS:${PN} += " openssl"

do_configure() {
  ./configure --host=${HOST_SYS}
}

do_install() {
  install -d ${D}${libdir}
  install libswitchtec.so ${D}${libdir}/libswitchtec.so
  ln -rs ${D}${libdir}/libswitchtec.so ${D}${libdir}/libswitchtec.so.2
  install -d ${D}${includedir}/switchtec
  install -m 0644 inc/switchtec/*.h ${D}${includedir}/switchtec/
  install -d ${D}${bindir}
  install -m 755 switchtec ${D}${bindir}/switchtec
}

FILES:${PN} = "${libdir}/libswitchtec.so ${libdir}/libswitchtec.so.2 ${bindir}/switchtec"
FILES:${PN}-dev = "${includedir}/switchtec"
