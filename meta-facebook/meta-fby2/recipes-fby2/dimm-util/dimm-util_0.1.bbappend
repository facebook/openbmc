# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += " file://me-functions.cpp \
             file://dimm-util-plat.h \
           "



CXXFLAGS += " -lbic"

DEPENDS += "libbic"
RDEPENDS:${PN} += "libbic"
