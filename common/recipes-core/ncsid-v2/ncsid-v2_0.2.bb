# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ncsid tx/rx daemon"
DESCRIPTION = "The NC-SI daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ncsid.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://ncsid.c \
    file://setup-ncsid.sh \
    file://enable-aen.sh \
    file://run-ncsid.sh \
    "

DEPENDS =+ " update-rc.d-native libpal libncsi obmc-libpldm libkv "

binfiles = "ncsid"

CFLAGS += " -lncsi -lobmc-pldm -lkv -lnl-wrapper"

pkgdir = "ncsid"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ncsid ${dst}/ncsid
  ln -snf ../fbpackages/${pkgdir}/ncsid ${bin}/ncsid

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ncsid
  install -d ${D}${sysconfdir}/ncsid
  install -m 755 setup-ncsid.sh ${D}${sysconfdir}/init.d/setup-ncsid.sh
  install -m 755 run-ncsid.sh ${D}${sysconfdir}/sv/ncsid/run
  install -m 755 enable-aen.sh ${bin}/enable-aen.sh
  update-rc.d -r ${D} setup-ncsid.sh start 91 5 .
}

DEPENDS += "libpal libncsi obmc-libpldm libkv libnl-wrapper"
DEPENDS += "update-rc.d-native"
RDEPENDS:${PN} = "libpal libncsi obmc-libpldm libkv libnl-wrapper"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/ncsid ${prefix}/local/bin ${sysconfdir}"
