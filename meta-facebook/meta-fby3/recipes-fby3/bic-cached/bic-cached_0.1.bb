# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Cache Tool"
DESCRIPTION = "Tool to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://setup-bic-cached.sh \
    file://bic-cached.c \
    "

CFLAGS += " -Wall -Werror "
LDFLAGS = "-lfby3_common -lbic -lpal"
DEPENDS = "libipmi libipmb libfby3-common libbic libpal update-rc.d-native"
RDEPENDS:${PN} = "libipmi libipmb libfby3-common libbic libpal"


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

FILES:${PN} = "${FBPACKAGEDIR}/bic-cached ${prefix}/local/bin ${sysconfdir} "
