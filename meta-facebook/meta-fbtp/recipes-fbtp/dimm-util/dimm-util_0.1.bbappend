# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += " file://me-functions.cpp \
             file://dimm-util-plat.h \
           "



CXXFLAGS += " -lipmb -lipmi -lpal"

DEPENDS += "libipmb libipmi libpal"
RDEPENDS:${PN} += "libipmb libipmi libpal"
