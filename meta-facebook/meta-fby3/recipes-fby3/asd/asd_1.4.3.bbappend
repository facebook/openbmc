# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libbic \
            libkv \
            libfby3-common \
            libipmi \
           "

RDEPENDS:${PN} += "libbic \
                   libfby3-common \
                  "

LDFLAGS =+ " -lkv -lfby3_common -lbic -lipmi "
