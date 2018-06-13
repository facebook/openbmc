SUMMARY = "Verified boot library"
DESCRIPTION = "This provides a library to read the verified boot status"
SECTION = "base"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://vbs.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI += " \
    file://vbs/Makefile \
    file://vbs/vbs.h \
    file://vbs/vbs.c \
    "

S = "${WORKDIR}/vbs"

do_install() {
  install -d ${D}${includedir}/openbmc
  install -m 0644 vbs.h ${D}${includedir}/openbmc/vbs.h

	install -d ${D}${libdir}
  install -m 0644 libvbs.so ${D}${libdir}/libvbs.so
}

FILES_${PN} = "${libdir}/libvbs.so"
FILES_${PN}-dev = "${includedir}/openbmc/vbs.h"
