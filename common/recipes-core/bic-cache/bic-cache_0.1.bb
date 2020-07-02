# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Cache Daemon"
DESCRIPTION = "Daemon to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-cache.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://Makefile \
           file://setup-bic-cache.sh \
           file://bic-cache.c \
          "

S = "${WORKDIR}"

LDFLAGS = "-lbic -lpal -llog"

binfiles = "bic-cache"

pkgdir = "bic-cache"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 bic-cache ${dst}/bic-cache
  ln -snf ../fbpackages/${pkgdir}/bic-cache ${bin}/bic-cache
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-bic-cache.sh ${D}${sysconfdir}/init.d/setup-bic-cache.sh
  update-rc.d -r ${D} setup-bic-cache.sh start 66 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/bic-cache ${prefix}/local/bin ${sysconfdir} "

DEPENDS += " libbic libpal liblog update-rc.d-native"
RDEPENDS_${PN} += " libbic libpal liblog"

# Inhibit complaints about .debug directories:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
