# Copyright 2014-present Facebook. All Rights Reserved.

SUMMARY = "OOB Shared NIC driver"
DESCRIPTION = "The shared-nic driver"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://main.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://src \
          "

S = "${WORKDIR}/src"

DEPENDS += "fbutils libwedge-eeprom"

RDEPENDS_${PN} += "libwedge-eeprom"

do_install() {
  install -d ${D}${sbindir}
  install -m 755 oob-nic ${D}${sbindir}/oob-nic
  install -m 755 i2craw ${D}${sbindir}/i2craw
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 etc/oob-nic.sh ${D}${sysconfdir}/init.d/oob-nic.sh
  update-rc.d -r ${D} oob-nic.sh start 80 S .
}

FILES_${PN} = " ${sbindir} ${sysconfdir} "
