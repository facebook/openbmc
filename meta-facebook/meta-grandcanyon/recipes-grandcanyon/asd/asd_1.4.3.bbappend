# Copyright 2021-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libbic \
            libkv \
            libfbgc-common \
            libipmi \
           "

LDFLAGS =+ " -lkv -lfbgc_common -lbic -lipmi "
