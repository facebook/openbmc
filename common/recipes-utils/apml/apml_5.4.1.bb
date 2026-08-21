# University of Illinois/NCSA Open Source License
#
# Copyright (c) 2020, Advanced Micro Devices, Inc.
# All rights reserved.

SUMMARY = "APML Library"
DESCRIPTION = "APML Library"

LICENSE = "NCSA"
LIC_FILES_CHKSUM = "file://License.txt;md5=a53f186511a093774907861d15f7014c"
# SRC_URI = "git://github.com/amd/esmi_oob_library.git;branch=master;protocol=https \
#          "
# SRCREV = "f47d1e620238f803f65bb33ed783d284580d5a9a"
SRC_URI = "file://apml"

PV = "5.4.1-f47d1e6"

S = "${UNPACKDIR}/apml"

inherit pkgconfig cmake

# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = ""
