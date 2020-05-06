# 2011-2020 Wiwynn Corporation. All Rights Reserved.
SUMMARY = "LAN Utility"
DESCRIPTION = "Util for communicating with LAN"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://lan-util.c;beginline=4;endline=16;md5=68b001f6d78951b122f3e673f833a332"

SRC_URI = "file://Makefile \
           file://if_parser.c \
           file://if_parser.h \
           file://net_lib.c \
           file://net_lib.h \
           file://strlib.c \
           file://strlib.h \
           file://lan-util.c \
           file://setup-init_net_interface.sh \
          "

S = "${WORKDIR}"

binfiles = "lan-util"

pkgdir = "lan-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 lan-util ${dst}/lan-util
  ln -snf ../fbpackages/${pkgdir}/lan-util ${bin}/lan-util

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-init_net_interface.sh ${D}${sysconfdir}/init.d/setup-init_net_interface.sh
  update-rc.d -r ${D} setup-init_net_interface.sh start 39 S .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/lan-util ${prefix}/local/bin ${sysconfdir}"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
