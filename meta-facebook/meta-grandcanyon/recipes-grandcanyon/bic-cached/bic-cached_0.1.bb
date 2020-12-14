# Copyright 2020-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Cache Tool"
DESCRIPTION = "Tool to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson

DEPENDS_append = " libbic libpal update-rc.d-native "
RDEPENDS_${PN} += " libbic libpal "

SRC_URI = "file://meson.build \
           file://setup-bic-cached.sh \
           file://bic-cached.c \
          "

S = "${WORKDIR}"

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 ${S}/setup-bic-cached.sh ${D}${sysconfdir}/init.d/setup-bic-cached.sh
  update-rc.d -r ${D} setup-bic-cached.sh start 66 5 .
}

