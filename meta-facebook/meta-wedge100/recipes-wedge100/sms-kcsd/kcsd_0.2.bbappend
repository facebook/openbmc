# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "KCS tx/rx daemon"
DESCRIPTION = "The KCS daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://kcsd.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://Makefile \
           file://kcsd.c \
           file://setup-kcsd.sh \
          "

S = "${WORKDIR}"
DEPENDS += "libipmi libgpio"

binfiles = "sms-kcsd"

pkgdir = "sms-kcsd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 kcsd ${dst}/kcsd
  ln -snf ../fbpackages/${pkgdir}/kcsd ${bin}/kcsd
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-kcsd.sh ${D}${sysconfdir}/init.d/setup-kcsd.sh
  update-rc.d -r ${D} setup-kcsd.sh start 65 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/sms-kcsd ${prefix}/local/bin ${sysconfdir} "

RDEPENDS_${PN} = "libipmi libgpio"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
