# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "fru-svc for OpenBMC"
DESCRIPTION = "This daemon provides fru services over dbus"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://FruSvcd.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

LOCAL_URI =+ " \
    file://Makefile \
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
    file://FruIdAccessMechanism.h \
    file://FruIdAccessI2CEEPROM.h \
    file://FruIdAccessI2CEEPROM.cpp \
    file://setup-fru-svcd.sh \
    file://run-fru-svcd.sh \
    "

LDFLAGS =+ "-lpthread -lgobject-2.0 -lobject-tree -lgflags -lgtest -lglog -lgio-2.0 -lglib-2.0 -ldbus-utils -lfruid"
DEPENDS =+ "nlohmann-json libipc object-tree dbus-utils gtest glog gflags libfruid update-rc.d-native"
RDEPENDS:${PN} += "dbus"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

FRU_SVC_INIT_FILE = "setup-fru-svcd.sh"
FRU_SVC_SV_FILE = "run-fru-svcd.sh"
FRU_SVC_DBUS_CONFIG = "org.openbmc.FruService.conf"
FRU_SVC_BIN_FILES = "fru-svcd"

do_install() {
  bin="${D}/usr/local/bin"
  install -d $bin
  install -d ${D}${sysconfdir}
  install -d ${D}${sysconfdir}/dbus-1
  install -d ${D}${sysconfdir}/dbus-1/system.d
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/fru-svcd

  for f in ${FRU_SVC_DBUS_CONFIG}; do
    install -m 644 $f ${D}${sysconfdir}/dbus-1/system.d/$f
  done

  if [ -z $FRU_SVC_INIT_FILE ]; then
    install -m 755 ${FRU_SVC_INIT_FILE} ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} ${FRU_SVC_INIT_FILE} start 91 5 .
  fi

  if [ -z $FRU_SVC_SV_FILE ]; then
    install -m 755 ${FRU_SVC_SV_FILE} ${D}${sysconfdir}/sv/fru-svcd/run
  fi

  for f in ${FRU_SVC_BIN_FILES}; do
    install -m 755 $f ${bin}/$f
  done
}

FILES:${PN} = "${prefix}/local/bin ${sysconfdir}"
