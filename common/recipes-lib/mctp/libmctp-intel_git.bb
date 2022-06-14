# Copyright 2020-present Facebook. All Rights Reserved.
HOMEPAGE = "https://github.com/Intel-BMC/libmctp"
SUMMARY = "MCTP stack"
DESCRIPTION = "MCTP library implementing the MCTP base specification"
PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=0d30807bb7a4f16d36e96b78f9ed8fae"

SRC_URI = "git://github.com/Intel-BMC/libmctp.git;protocol=https;branch=master \
           file://0001-Remove-unknown-i2c-smbus.h.patch \
           file://0002-Build-as-shared-library.patch \
           file://0003-Customize-buffer-size.patch \
           file://0004-test-astlpc-fix-compile-warning.patch \
          "
SRCREV = "d530c2271e1f9ff5d76a170c0abd64bd03ef40fd"
SRC_URI[md5sum] = "1c7db87fb8fa946b03d91fff08b0825a"
SRC_URI[sha256sum] = "f7d36674d1e72d216fe60d804fc55ba5284162e8ec6a79e6ef2e067d3aee7675"

inherit pkgconfig cmake

S = "${WORKDIR}/git"

do_configure:append() {
  cd ${S}
  cmake
}

do_install:append() {
  install -d ${D}${libdir}
  install libmctp_intel.so ${D}${libdir}/libmctp_intel.so
  ln -rs ${D}${libdir}/libmctp_intel.so ${D}${libdir}/libmctp_intel.so.0
  install -d ${D}${includedir}
  install -m 0644 ${S}/*.h ${D}${includedir}/
}

FILES:${PN} = "${libdir}/libmctp_intel.so ${libdir}/libmctp_intel.so.0"
FILES:${PN}-dev = "${includedir}/*.h"
