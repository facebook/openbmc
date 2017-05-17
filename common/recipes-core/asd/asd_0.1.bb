# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "At Scale Debug daemon"
DESCRIPTION = "The asd daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://socket_main.c;beginline=6;endline=13;md5=678b5e5a5bd177043fb4177e772c804d"

SRC_URI = "file://daemon \
          "

DEPENDS += "libasd-jtagintf"
RDEPENDS_${PN} += "libasd-jtagintf"

S = "${WORKDIR}/daemon"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 asd ${dst}/asd
  ln -snf ../fbpackages/${pkgdir}/asd ${bin}/asd
#  install -d ${D}${sysconfdir}/init.d
#  install -d ${D}${sysconfdir}/rcS.d
#  install -m 755 setup-asd.sh ${D}${sysconfdir}/init.d/setup-asd.sh
#  update-rc.d -r ${D} setup-asd.sh start 65 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES_${PN} = "${FBPACKAGEDIR}/asd ${prefix}/local/bin ${sysconfdir} "

