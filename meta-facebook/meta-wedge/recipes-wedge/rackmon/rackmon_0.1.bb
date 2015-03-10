# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Rackmon Functionality"
DESCRIPTION = "Rackmon Functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://modbus.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

#DEPENDS_append = " update-rc.d-native"

SRC_URI = "file://Makefile \
           file://modbuscmd.c \
           file://modbussim.c \
           file://modbus.c \
           file://modbus.h \
           file://gpiowatch.c \
          "

S = "${WORKDIR}"

binfiles = "modbuscmd \
            modbussim \
            gpiowatch \
           "

#otherfiles = "README"

pkgdir = "rackmon"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rackmon ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories for the rackmon binaries:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
