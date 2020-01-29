# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Cache Daemon"
DESCRIPTION = "Daemon to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://Makefile \
           file://setup-bic-cached.sh \
           file://bic-cached.c \
          "

S = "${WORKDIR}"

LDFLAGS = "-lbic -lpal -llog"

binfiles = "bic-cached"

pkgdir = "bic-cached"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 bic-cached ${dst}/bic-cached
  ln -snf ../fbpackages/${pkgdir}/bic-cached ${bin}/bic-cached
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-bic-cached.sh ${D}${sysconfdir}/init.d/setup-bic-cached.sh
  update-rc.d -r ${D} setup-bic-cached.sh start 66 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/bic-cached ${prefix}/local/bin ${sysconfdir} "

DEPENDS += " libbic libpal liblog update-rc.d-native"
RDEPENDS_${PN} += " libbic libpal liblog"

# Inhibit complaints about .debug directories:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
