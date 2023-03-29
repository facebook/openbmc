# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libkv \
            libipmi \
            libgpio-ctrl \
           "

LDFLAGS =+ " -lkv -lipmi -lgpio-ctrl "
