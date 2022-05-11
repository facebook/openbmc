# Copyright 2021-present Facebook. All Rights Reserved.
SUMMARY = "System Event Gen Utility"
DESCRIPTION = "Utility for creating and injecting system events"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://syseventgen-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://syseventgen-util.cpp \
    file://syseventgen-util.h \
    "

DEPENDS += "nlohmann-json libpal cli11 libsdr libipmi"
RDEPENDS:${PN} += "libpal libsdr libipmi"

FILES:${PN} = "${prefix}/local/bin/syseventgen-util"
