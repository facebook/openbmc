# Copyright 2022-present Facebook. All Rights Reserved.
SUMMARY = "Platform Root of Trust Utility"
DESCRIPTION = "Util for communicating to Platfire"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://prot-util.cpp;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = " \
    file://prot-util \
    "
S = "${WORKDIR}/prot-util"

inherit meson pkgconfig

DEPENDS += "cli11 nlohmann-json libpal libprot ami-smbus-intf fmt"
