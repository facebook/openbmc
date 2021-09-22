# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libbic \
            libkv \
            libfby35-common \
            libipmi \
           "

RDEPENDS:${PN} += "libbic \
                   libkv  \
                   libfby35-common \
                   libipmi \
                  "

LDFLAGS =+ " -lkv -lfby35_common -lbic -lipmi "
