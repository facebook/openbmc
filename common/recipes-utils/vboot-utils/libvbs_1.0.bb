SUMMARY = "Verified boot library"
DESCRIPTION = "This provides a library to read the verified boot status"
SECTION = "base"
LICENSE = "GPL-2.0-or-later"
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

def get_vbs_base_addr(soc_family):
  ret = "-DAST_SRAM_VBS_BASE="

  if soc_family == "aspeed-g4":
    ret += "0x1E720200"
  elif soc_family == "aspeed-g5":
    ret += "0x1E720200"
  elif soc_family == "aspeed-g6":
    ret += "0x10015800"
  else:
    ret = ""

  return ret


CFLAGS += "${@ get_vbs_base_addr('${SOC_FAMILY}') }"
FILES:${PN} = "${libdir}/libvbs.so"
FILES:${PN}-dev = "${includedir}/openbmc/vbs.h"
