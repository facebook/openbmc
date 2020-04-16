# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "eMMC Daemon"
DESCRIPTION = "Daemon to monitor eMMC status "
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://emmcd.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = "update-rc.d-native"

SRC_URI = "file://Makefile \
           file://emmcd.c \
           file://setup-emmcd.sh \
           file://run-emmcd.sh \
          "

S = "${WORKDIR}"

binfiles = "emmcd"

pkgdir = "emmcd"

do_install() {
dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 emmcd ${dst}/emmcd
  ln -snf ../fbpackages/${pkgdir}/emmcd ${bin}/emmcd

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/emmcd
  install -d ${D}${sysconfdir}/emmcd
  install -m 755 setup-emmcd.sh ${D}${sysconfdir}/init.d/setup-emmcd.sh
  install -m 755 run-emmcd.sh ${D}${sysconfdir}/sv/emmcd/run
  update-rc.d -r ${D} setup-emmcd.sh start 99 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/emmcd ${prefix}/local/bin ${sysconfdir} "

