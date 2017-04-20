# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "ECC Monitor Daemon"
DESCRIPTION = "Daemon to monitor ECC register"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ecc-mond.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = "libpal update-rc.d-native"

SRC_URI = "file://Makefile \
           file://setup-ecc-mond.sh \
           file://ecc-mond.c \
           file://run-ecc-mond.sh \
          "

S = "${WORKDIR}"

binfiles = "ecc-mond"

pkgdir = "ecc-mond"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ecc-mond ${dst}/ecc-mond
  ln -snf ../fbpackages/${pkgdir}/ecc-mond ${bin}/ecc-mond

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ecc-mond
  install -d ${D}${sysconfdir}/ecc-mond
  install -m 755 setup-ecc-mond.sh ${D}${sysconfdir}/init.d/setup-ecc-mond.sh
  install -m 755 run-ecc-mond.sh ${D}${sysconfdir}/sv/ecc-mond/run
  update-rc.d -r ${D} setup-ecc-mond.sh start 96 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN}-dbg += "${FBPACKAGEDIR}/ecc-mond/.debug"
FILES_${PN} += "${FBPACKAGEDIR}/ecc-mond ${prefix}/local/bin"
