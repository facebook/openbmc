# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "SCC Expander Cache Daemon"
DESCRIPTION = "Daemon to provide SCC Expander Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://exp-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = "libpal libexp update-rc.d-native"

SRC_URI = "file://Makefile \
           file://setup-exp-cached.sh \
           file://exp-cached.c \
          "

S = "${WORKDIR}"

binfiles = "exp-cached"

pkgdir = "exp-cached"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 exp-cached ${dst}/exp-cached
  ln -snf ../fbpackages/${pkgdir}/exp-cached ${bin}/exp-cached
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-exp-cached.sh ${D}${sysconfdir}/init.d/setup-exp-cached.sh
  update-rc.d -r ${D} setup-exp-cached.sh start 71 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/exp-cached ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
