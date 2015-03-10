# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "SMS KCS Daemon"
DESCRIPTION = "Daemon to handle SMS KCS interface."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sms-kcsd.c;beginline=12;endline=24;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = " update-rc.d-native"

DEPENDS += "libalert-control"
DEPENDS += "libipmi"

SRC_URI = "file://Makefile \
           file://setup-sms-kcs.sh \
           file://sms-kcsd.c \
          "

S = "${WORKDIR}"

binfiles = "sms-kcsd"

pkgdir = "sms-kcsd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 sms-kcsd ${dst}/sms-kcsd
  ln -snf ../fbpackages/${pkgdir}/sms-kcsd ${bin}/sms-kcsd
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-sms-kcs.sh ${D}${sysconfdir}/init.d/setup-sms-kcs.sh
  update-rc.d -r ${D} setup-sms-kcs.sh start 65 S .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/sms-kcsd ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories for the fand binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
