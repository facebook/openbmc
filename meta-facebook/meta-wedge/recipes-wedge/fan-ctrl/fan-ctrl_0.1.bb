# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Fan controller"
DESCRIPTION = "The utilities to control fan."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fand.cpp;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS_append = "libwedge-eeprom update-rc.d-native"

SRC_URI = "file://get_fan_speed.sh \
           file://init_pwm.sh \
           file://set_fan_speed.sh \
           file://README \
           file://setup-fan.sh \
           file://Makefile \
           file://fand.cpp \
           file://watchdog.h \
           file://watchdog.cpp \
          "

S = "${WORKDIR}"

binfiles = "get_fan_speed.sh \
            init_pwm.sh \
            set_fan_speed.sh \
	    fand \
           "

otherfiles = "README"

pkgdir = "fan_ctrl"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  for f in ${otherfiles}; do
    install -m 644 $f ${dst}/$f
  done
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-fan.sh ${D}${sysconfdir}/init.d/setup-fan.sh
  update-rc.d -r ${D} setup-fan.sh start 91 S .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/fan_ctrl ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories for the fand binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
