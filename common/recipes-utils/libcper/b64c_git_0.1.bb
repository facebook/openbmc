SUMMARY = "b64.c"
DESCRIPTION = "Base64 encode/decode"
HOMEPAGE = "https://github.com/jwerle/b64.c"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=85fb8f8c4178910b7d79adf29af6cdb1"

SRC_URI = "git://github.com/jwerle/b64.c;branch=master;protocol=https \
          "

# b64.c 9/2/2021
SRCREV = "c33188cd541f19b072ee4988d8224ea6c964bed1"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

do_install:append() {
  install -d ${D}${libdir}
  install libb64c.so ${D}${libdir}/libb64c.so

  install -d ${D}${includedir}
  install -m 0644 ${S}/*.h ${D}${includedir}/
}

FILES:${PN} = "${libdir}/libb64c.so"
FILES:${PN}-dev = "${includedir}/*.h"