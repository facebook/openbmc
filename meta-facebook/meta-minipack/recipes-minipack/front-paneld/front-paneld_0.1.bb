# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "Front Panel Control Daemon"
DESCRIPTION = "Daemon to monitor and control the front panel "
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://front-paneld.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = "libpal libbic libkv libsdr update-rc.d-native"

SRC_URI = "file://Makefile \
           file://setup-front-paneld.sh \
           file://front-paneld.c \
           file://run-front-paneld.sh \
          "

S = "${WORKDIR}"

binfiles = "front-paneld"

pkgdir = "front-paneld"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 front-paneld ${dst}/front-paneld
  ln -snf ../fbpackages/${pkgdir}/front-paneld ${bin}/front-paneld
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/front-paneld
  install -d ${D}${sysconfdir}/front-paneld
  install -m 755 setup-front-paneld.sh ${D}${sysconfdir}/init.d/setup-front-paneld.sh
  install -m 755 run-front-paneld.sh ${D}${sysconfdir}/sv/front-paneld/run
  update-rc.d -r ${D} setup-front-paneld.sh start 67 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/front-paneld ${prefix}/local/bin ${sysconfdir} "
RDEPENDS_${PN} += " libpal libbic libkv libsdr "


# Inhibit complaints about .debug directories:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
