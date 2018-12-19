# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "PLDM Library"
DESCRIPTION = "library to provide PLDM functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://pldm.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://pldm.c \
           file://pldm.h \
           file://pldm_base.h \
           file://pldm_fw_update.h \
          "

S = "${WORKDIR}"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libpldm.so ${D}${libdir}/libpldm.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 pldm.h ${D}${includedir}/openbmc/pldm.h
    install -m 0644 pldm_base.h ${D}${includedir}/openbmc/pldm_base.h
    install -m 0644 pldm_fw_update.h ${D}${includedir}/openbmc/pldm_fw_update.h
}

DEPENDS =+ "libncsi"
RDEPENDS_${PN} =+ "libncsi"

FILES_${PN} = "${libdir}/libpldm.so"
FILES_${PN}-dev = "${includedir}/openbmc/*"
