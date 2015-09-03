# Copyright 2014-present Facebook. All Rights Reserved.

SUMMARY = "LLDP Utility"
DESCRIPTION = "A utility for reporting LLDP information"
SECTION = "base"
PR = "r1"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=cc7ed73f378cc0ea228aebab24237853 \
                    file://PATENTS;md5=aed2575e5dba9ba3aea25cfeddb8f1d2 "

SRC_URI = "file://src \
          "

S = "${WORKDIR}/src"

do_install() {
  install -d ${D}${bindir}
  install -m 755 lldp-util ${D}${bindir}
}

FILES_${PN} = " ${bindir} "
