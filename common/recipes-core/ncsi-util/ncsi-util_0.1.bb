# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NCSI Utility"
DESCRIPTION = "Util for sending NC-SI command to NIC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ncsi-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://ncsi-util.c \
    file://ncsi-util.h \
    file://brcm-ncsi-util.c \
    file://brcm-ncsi-util.h \
    file://nvidia-ncsi-util.c \
    file://nvidia-ncsi-util.h \
    "

binfiles = "ncsi-util"

LDFLAGS += "-lpal -lncsi -lobmc-pldm -lnl-wrapper -lkv -lz"

pkgdir = "ncsi-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ncsi-util ${dst}/ncsi-util
  ln -snf ../fbpackages/${pkgdir}/ncsi-util ${bin}/ncsi-util
}

DEPENDS += "libpal libncsi obmc-libpldm libnl-wrapper libkv zlib"
RDEPENDS:${PN} += "libpal libncsi obmc-libpldm libnl-wrapper libkv zlib"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/ncsi-util ${prefix}/local/bin"
