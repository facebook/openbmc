# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Platform Service for OpenBMC"
DESCRIPTION = "This deamon takes input from Json file and builds platform tree. It pushes data to respective dbus services (for eg SensorService)"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://PlatformSvcd.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

LOCAL_URI =+ " \
    file://Makefile \
    file://org.openbmc.PlatformService.conf \
    file://Sensor.h \
    file://PlatformObjectTree.cpp \
    file://PlatformObjectTree.h \
    file://PlatformJsonParser.cpp \
    file://PlatformService.h \
    file://PlatformJsonParser.h \
    file://PlatformSvcd.cpp \
    file://FRU.h \
    file://SensorService.h \
    file://SensorService.cpp \
    file://DBusPlatformSvcInterface.cpp \
    file://DBusPlatformSvcInterface.h \
    file://FruService.h \
    file://FruService.cpp \
    file://HotPlugDetectionMechanism.h \
    file://HotPlugDetectionViaPath.h \
    file://DBusHPExtDectectionFruInterface.h \
    file://DBusHPExtDectectionFruInterface.cpp \
    "

DEPENDS =+ "nlohmann-json libipc object-tree dbus-utils glog gflags"
RDEPENDS:${PN} += "dbus"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

binfiles = "platform-svcd"
pkgdir = "platform-svc"

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
  install -m 644 org.openbmc.PlatformService.conf ${D}${sysconfdir}/dbus-1/system.d/org.openbmc.PlatformService.conf
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/platform-svc ${prefix}/local/bin ${sysconfdir}"
