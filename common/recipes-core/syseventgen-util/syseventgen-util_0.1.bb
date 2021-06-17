# Copyright 2021-present Facebook. All Rights Reserved.
SUMMARY = "System Event Gen Utility"
DESCRIPTION = "Utility for creating and injecting system events"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://syseventgen-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit meson

SRC_URI =  "file://meson.build \
            file://syseventgen-util.cpp \
            file://syseventgen-util.h \
            "

S = "${WORKDIR}"
DEPENDS += "nlohmann-json libpal cli11 libsdr"
RDEPENDS_${PN} += "libpal libsdr"

FILES_${PN} = "${prefix}/local/bin/syseventgen-util"
