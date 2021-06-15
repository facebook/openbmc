# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"

DEPENDS += "libbic \
            libkv \
            libfby35-common \
            libipmi \
           "

RDEPENDS_${PN} += "libbic \
                   libkv  \
                   libfby35-common \
                   libipmi \
                  "

LDFLAGS =+ " -lkv -lfby35_common -lbic -lipmi "
