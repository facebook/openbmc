# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "DBus utility to print object tree at dbus Service"
DESCRIPTION = "Introspect dbus service and print object tree"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://DBusPrintObjectTree.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

SRC_URI = "file://CMakeLists.txt \
           file://DBusPrintObjectTree.cpp \
          "

S = "${WORKDIR}"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

inherit cmake

DEPENDS += "glib-2.0"
