# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NCSI Utility"
DESCRIPTION = "Util for sending NC-SI command to NIC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ncsi-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://Makefile \
           file://ncsi-util.c \
           file://libnl-wrapper.c \
           file://libnl-wrapper.h \
          "

S = "${WORKDIR}"
binfiles = "ncsi-util"

CFLAGS += " -lpal -lncsi -lpldm  -lnl-3 -lnl-cli-3 -lnl-genl-3 -lnl-nf-3 -I${STAGING_INCDIR}/libnl3"

pkgdir = "ncsi-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ncsi-util ${dst}/ncsi-util
  ln -snf ../fbpackages/${pkgdir}/ncsi-util ${bin}/ncsi-util
}

DEPENDS += "libpal libncsi libpldm libnl"
RDEPENDS_${PN} += "libpal libncsi libpldm libnl"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/ncsi-util ${prefix}/local/bin"
