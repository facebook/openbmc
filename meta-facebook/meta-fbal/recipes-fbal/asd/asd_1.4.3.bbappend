# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"

DEPENDS += "libkv \
            libipmi \
            libgpio-ctrl \
           "

RDEPENDS_${PN} += "libkv  \
                   libipmi \
                   libgpio-ctrl \
                  "

LDFLAGS =+ " -lkv -lipmi -lgpio-ctrl "
