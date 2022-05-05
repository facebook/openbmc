SUMMARY = "PECI Library"
DESCRIPTION = "PECI Library"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7becf906c8f8d03c237bad13bc3dac53"
inherit cmake

SRC_URI = "git://github.com/openbmc/libpeci;branch=master;protocol=https"

PV = "0.1+git${SRCPV}"
SRCREV = "c965e72c6765e054531c1ab91e7fa13f04651f21"

S = "${WORKDIR}/git"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libpeci.so.1 ${D}${libdir}/libpeci.so.1
  ln -sf libpeci.so.1 ${D}${libdir}/libpeci.so

  install -d ${D}${includedir}
  install -m 0644 ${S}/peci.h ${D}${includedir}/peci.h
  install -d ${D}${includedir}/linux
  install -m 0644 ${S}/linux/peci-ioctl.h ${D}${includedir}/linux/peci-ioctl.h
}

FILES:${PN} = "${libdir}/libpeci.so ${libdir}/libpeci.so.1"
INSANE_SKIP:${PN} += "dev-so"
FILES:${PN}-dev = "${includedir}/peci.h ${includedir}/linux/peci-ioctl.h"
