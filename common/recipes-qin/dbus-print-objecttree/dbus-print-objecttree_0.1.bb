# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "DBus utility to print object tree at dbus Service"
DESCRIPTION = "Introspect dbus service and print object tree"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://DBusPrintObjectTree.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

LOCAL_URI = " \
    file://CMakeLists.txt \
    file://DBusPrintObjectTree.cpp \
    "

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

inherit cmake

DEPENDS += "glib-2.0"
