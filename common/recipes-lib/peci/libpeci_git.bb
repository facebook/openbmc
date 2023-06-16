SUMMARY = "PECI Library"
DESCRIPTION = "PECI Library"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7becf906c8f8d03c237bad13bc3dac53"
inherit cmake

SRC_URI = "git://github.com/openbmc/libpeci;branch=master;protocol=https \
           file://0001-remove_inital_log.patch \
           file://0002-Fix-GCC-13-issue.patch \
           "

PV = "0.1+git${SRCPV}"
SRCREV = "6a00e9aa72f75d66eb8b9572c7fd3894f91c6bba"

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
