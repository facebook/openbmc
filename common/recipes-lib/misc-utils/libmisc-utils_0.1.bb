# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

SUMMARY = "Miscellaneous Utilities Library"
DESCRIPTION = "Miscellaneous Utilities Library"

SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://misc-utils.h;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"
BBCLASSEXTEND = "native"

inherit ptest

SRC_URI = "file://Makefile \
           file://device-utils.c \
           file://file-utils.c \
           file://path-utils.c \
           file://plat-utils.c \
           file://str-utils.c \
           file://misc-utils.h \
           "

# Add Test sources
SRC_URI += "file://test/main.c \
           file://test/test-defs.h \
           file://test/test-file.c \
           file://test/test-path.c \
           file://test/test-str.c \
           "
S = "${WORKDIR}"

do_compile_ptest() {
  make test-libmisc-utils
  cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
/usr/lib/libmisc-utils/ptest/test-libmisc-utils
EOF
}

def get_soc_model(soc_family):
  if soc_family == "aspeed-g4":
    ret = "SOC_MODEL_ASPEED_G4"
  elif soc_family == "aspeed-g5":
    ret = "SOC_MODEL_ASPEED_G5"
  elif soc_family == "aspeed-g6":
    ret = "SOC_MODEL_ASPEED_G6"
  else:
    ret = "SOC_MODEL_INVALID"

  return ret

def get_cpu_model(soc_family):
  if soc_family == "aspeed-g4":
    ret = "CPU_MODEL_ARM_V5"
  elif soc_family == "aspeed-g5":
    ret = "CPU_MODEL_ARM_V6"
  elif soc_family == "aspeed-g6":
    ret = "CPU_MODEL_ARM_V7"
  else:
    ret = "CPU_MODEL_INVALID"

  return ret

CFLAGS += "-DSOC_MODEL=${@ get_soc_model('${SOC_FAMILY}') }"
CFLAGS += "-DCPU_MODEL=${@ get_cpu_model('${SOC_FAMILY}') }"

do_install_ptest() {
  install -d ${D}${libdir}/libmisc-utils
  install -d ${D}${libdir}/libmisc-utils/ptest
  install -m 755 test-libmisc-utils ${D}${libdir}/libmisc-utils/ptest/test-libmisc-utils
}

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libmisc-utils.so ${D}${libdir}/libmisc-utils.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 misc-utils.h ${D}${includedir}/openbmc/misc-utils.h

    install -d ${D}${sysconfdir}
    echo "${@ get_soc_model('${SOC_FAMILY}') }" > ${D}${sysconfdir}/soc_model
    echo "${@ get_cpu_model('${SOC_FAMILY}') }" > ${D}${sysconfdir}/cpu_model
}

FILES_${PN} = "${libdir}/libmisc-utils.so"
FILES_${PN} += "${sysconfdir}/soc_model"
FILES_${PN} += "${sysconfdir}/cpu_model"
FILES_${PN}-dev = "${includedir}/openbmc/misc-utils.h"
FILES_${PN}-ptest = "${libdir}/libmisc-utils/ptest/test-libmisc-utils ${libdir}/libmisc-utils/ptest/run-ptest"
