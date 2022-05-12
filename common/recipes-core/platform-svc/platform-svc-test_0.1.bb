# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Unit tests for Platform Service"
DESCRIPTION = "Unit tests for Platform Service"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://FRUTest.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"


SRC_URI =+ "file://tests \
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

S = "${WORKDIR}/tests"

DEPENDS = "glog gtest"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

inherit cmake
