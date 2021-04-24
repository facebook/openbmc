# Copyright 2021-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"

DEPENDS += "libbic \
            libkv \
            libfbgc-common \
            libipmi \
           "

RDEPENDS_${PN} += "libbic \
                   libkv  \
                   libfbgc-common \
                   libipmi \
                  "

LDFLAGS =+ " -lkv -lfbgc_common -lbic -lipmi "
