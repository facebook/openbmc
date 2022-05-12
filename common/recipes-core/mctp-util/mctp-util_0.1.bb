# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "MCTP Utility"
DESCRIPTION = "Util for sending MCTP command to a device"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mctp-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://decode.cpp \
    file://decode.h \
    file://mctp-util.cpp \
    file://mctp-util.h \
    "

binfiles = "mctp-util"

LDFLAGS += "-lpal -lobmc-mctp"

pkgdir = "mctp-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 mctp-util ${dst}/mctp-util
  ln -snf ../fbpackages/${pkgdir}/mctp-util ${bin}/mctp-util
}

DEPENDS += "libpal libobmc-mctp"
RDEPENDS:${PN} += "libpal libobmc-mctp"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/mctp-util ${prefix}/local/bin"
