# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ELBERT LED Control Daemon"
DESCRIPTION = "Daemon to change ELBERT System LEDs based on system condition"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://led-controld.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS:append = "libpal update-rc.d-native"

LOCAL_URI = " \
    file://Makefile \
    file://setup-led-controld.sh \
    file://led-controld.c \
    file://led-controld.h \
    file://run-led-controld.sh \
    "

binfiles = "led-controld"

pkgdir = "led-controld"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 led-controld ${dst}/led-controld
  ln -snf ../fbpackages/${pkgdir}/led-controld ${bin}/led-controld
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/led-controld
  install -d ${D}${sysconfdir}/led-controld
  install -m 755 setup-led-controld.sh ${D}${sysconfdir}/init.d/setup-led-controld.sh
  install -m 755 run-led-controld.sh ${D}${sysconfdir}/sv/led-controld/run
  update-rc.d -r ${D} setup-led-controld.sh start 67 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/led-controld ${prefix}/local/bin ${sysconfdir} "
RDEPENDS:${PN} += "libpal"
