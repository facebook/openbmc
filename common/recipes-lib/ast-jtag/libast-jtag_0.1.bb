# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "ASPEED JTAG Library"
DESCRIPTION = "Library for communicating with ASPEED JTAG controller driver (kernel 5.0.3)"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ast-jtag.c;beginline=8;endline=20;md5=435eababecd3f367d90616c70e27bdd6"

SRC_URI = "file://ast-jtag.c \
           file://ast-jtag.h \
           file://ast-jtag-intf.h \
           file://ast-jtag-intf.c \
           file://ast-jtag-legacy.c \
           file://jtag.h \
           file://meson.build \
          "

S = "${WORKDIR}"

inherit meson
