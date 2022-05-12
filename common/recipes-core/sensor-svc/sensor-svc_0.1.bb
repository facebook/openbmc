# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Sensord for OpenBMC"
DESCRIPTION = "This is a sensor daemon utilizing dbus-utils for IPC and object-tree for resource management"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://SensorSvcd.cpp;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

LOCAL_URI =+ " \
    file://Makefile \
    file://org.openbmc.SensorService.conf \
    file://Sensor.h \
    file://SensorAccessMechanism.h \
    file://SensorAccessVR.h \
    file://SensorAccessVR.cpp \
    file://DBusSensorInterface.cpp \
    file://SensorAccessAVA.h \
    file://Sensor.cpp \
    file://SensorObjectTree.cpp \
    file://DBusSensorInterface.h \
    file://SensorAccessINA230.h \
    file://SensorAccessINA230.cpp \
    file://SensorObjectTree.h \
    file://DBusSensorTreeInterface.cpp \
    file://SensorAccessViaPath.cpp \
    file://SensorJsonParser.cpp \
    file://SensorService.h \
    file://DBusSensorTreeInterface.h \
    file://SensorAccessNVME.h \
    file://SensorAccessNVME.cpp \
    file://SensorAccessViaPath.h \
    file://SensorJsonParser.h \
    file://SensorSvcd.cpp \
    file://FRU.h \
    file://SensorAccessMechanism.cpp \
    file://SensorAccessAVA.cpp \
    file://FRU.cpp \
    file://DBusSensorServiceInterface.cpp \
    file://DBusSensorServiceInterface.h \
    "

LDFLAGS =+ " -lpthread -lgobject-2.0 -lobject-tree -lgflags -lgtest -lglog -lgio-2.0 -lglib-2.0 -ldbus-utils -lobmc-i2c"
DEPENDS =+ "nlohmann-json libipc object-tree dbus-utils gtest glog gflags libobmc-i2c"
RDEPENDS:${PN} += "dbus libobmc-i2c"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

binfiles = "sensor-svcd"
pkgdir = "sensor-svc"

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
  install -m 644 org.openbmc.SensorService.conf ${D}${sysconfdir}/dbus-1/system.d/org.openbmc.SensorService.conf
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/sensor-svc ${prefix}/local/bin ${sysconfdir}"
