# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ME Cache Daemon"
DESCRIPTION = "Daemon to provide ME Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://me-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = "libme update-rc.d-native"

SRC_URI = "file://Makefile \
           file://setup-me-cached.sh \
           file://me-cached.c \
          "

S = "${WORKDIR}"

binfiles = "me-cached"

pkgdir = "me-cached"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 me-cached ${dst}/me-cached
  ln -snf ../fbpackages/${pkgdir}/me-cached ${bin}/me-cached
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-me-cached.sh ${D}${sysconfdir}/init.d/setup-me-cached.sh
  update-rc.d -r ${D} setup-me-cached.sh start 66 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/me-cached ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
