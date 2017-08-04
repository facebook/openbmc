# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "Unit tests for Platform Service"
DESCRIPTION = "Unit tests for Platform Service"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://FRUTest.cpp;beginline=4;endline=18;md5=3952f9784a9631138c6d1102e8b9a59d"


SRC_URI =+ "file://tests \
           file://FRU.h \
           file://HotPlugDetectionMechanism.h \
           file://HotPlugDetectionViaPath.h \
          "

S = "${WORKDIR}/tests"

DEPENDS = "glog gtest"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

inherit cmake
