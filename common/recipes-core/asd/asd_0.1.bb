# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "At Scale Debug daemon"
DESCRIPTION = "The asd daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://socket_main.c;beginline=4;endline=25;md5=4d3dd6a70786d475883b1542b0898219"

SRC_URI = "file://daemon \
          "

DEPENDS += "libasd-jtagintf libpal"
RDEPENDS:${PN} += "libasd-jtagintf libpal"

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
FILES:${PN} = "${FBPACKAGEDIR}/asd ${prefix}/local/bin ${sysconfdir} "

