# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "fru-svc for OpenBMC"
DESCRIPTION = "This daemon provides fru services over dbus"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://FruSvcd.cpp;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

SRC_URI =+ "file://Makefile \
           file://org.openbmc.FruService.conf \
           file://DBusFruInterface.cpp \
           file://DBusFruInterface.h \
           file://FruObjectTree.cpp \
           file://FruObjectTree.h \
           file://FruJsonParser.h \
           file://FruService.h \
           file://FruSvcd.cpp \
           file://FRU.h \
           file://DBusFruServiceInterface.cpp \
           file://DBusFruServiceInterface.h \
          "

S = "${WORKDIR}"

LDFLAGS =+ " -lpthread -lgobject-2.0 -lobject-tree -lgflags -lgtest -lglog -lgio-2.0 -lglib-2.0 -ldbus-utils"
DEPENDS =+ "nlohmann-json libipc object-tree dbus-utils gtest glog gflags"
RDEPENDS_${PN} += "dbus"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

binfiles = "fru-svcd"
pkgdir = "fru-svc"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done

  install -d ${D}${sysconfdir}/dbus-1
  install -d ${D}${sysconfdir}/dbus-1/system.d
  install -m 644 org.openbmc.FruService.conf ${D}${sysconfdir}/dbus-1/system.d/org.openbmc.FruService.conf
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/fru-svc ${prefix}/local/bin ${sysconfdir}"
