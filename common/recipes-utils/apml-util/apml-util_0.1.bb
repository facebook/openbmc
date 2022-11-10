# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "APML Utility"
DESCRIPTION = "Util for communicating to SOC PSP"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit meson pkgconfig
inherit legacy-packages

LOCAL_URI = " \
    file://apml-util.cpp \
    file://apml-extra.h \
    file://meson.build \
    "

pkgdir = "apml-util"

DEPENDS = " libbic"
RDEPENDS:${PN} += "libbic"
