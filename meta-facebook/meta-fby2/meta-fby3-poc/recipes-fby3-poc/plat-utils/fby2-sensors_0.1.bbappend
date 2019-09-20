# Copyright 2015-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_prepend := "${THISDIR}/files/:"

SRC_URI  += "file://fby2-sensors \
            "

LDFLAGS += " -lbic -lm "
DEPENDS += " libbic "
RDEPENDS_${PN} += " libbic "

CFLAGS_prepend = " -DCONFIG_FBY3_POC "
