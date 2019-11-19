# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += " file://me-functions.cpp \
             file://dimm-util-plat.h \
           "



CXXFLAGS += " -lipmb -lipmi -lpal"

DEPENDS += "libipmb libipmi libpal"
RDEPENDS_${PN} += "libipmb libipmi libpal"
