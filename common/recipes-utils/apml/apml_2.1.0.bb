# University of Illinois/NCSA Open Source License
#
# Copyright (c) 2020, Advanced Micro Devices, Inc.
# All rights reserved.

SUMMARY = "APML Library"
DESCRIPTION = "APML Library"

LICENSE = "NCSA"
LIC_FILES_CHKSUM = "file://License.txt;md5=a53f186511a093774907861d15f7014c"

SRC_URI = "git://github.com/amd/esmi_oob_library.git;branch=master;protocol=https \
           file://0001-add-amd-apml.h.patch \
          "

PV = "2.1.0"
SRCREV = "00cc0fb0265af1d240a0aff5ed96f90a73ff8c51"

S = "${WORKDIR}/git"


inherit pkgconfig cmake

# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = ""
