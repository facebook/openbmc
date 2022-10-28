SUMMARY = "Verified boot library"
DESCRIPTION = "This provides a library to read the verified boot status"
SECTION = "base"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://vbs.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

SRC_URI += "file://vbs"
S = "${WORKDIR}/vbs"

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
