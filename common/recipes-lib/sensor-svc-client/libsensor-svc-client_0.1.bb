# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing common sensor information access"
DESCRIPTION = "Library providing common sensor access"
SECTION = "libs"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://sensor-svc-client.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://CMakeLists.txt \
    file://sensor-svc-client.c \
    file://sensor-svc-client.h \
    "

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

DEPENDS =+ "glib-2.0"

inherit cmake
