# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"

DEPENDS += "libbic \
            libkv \
            libfby3-common \
            libipmi \
           "

RDEPENDS_${PN} += "libbic \
                   libkv  \
                   libfby3-common \
                   libipmi \
                  "

LDFLAGS =+ " -lkv -lfby3_common -lbic -lipmi "
