#
# Copyright (c) 2022 by Cisco Systems, Inc.
# All rights reserved.
#
SUMMARY = "cisco 8000 kernel modules"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${S}/git/LICENSE;md5=796c1766b2a94646dcb6a2a6a1f9be2b"

inherit module

PR = "r0"
PV = "1.17"

SRC_URI = " \
    file://Makefile \
    git://git@github.com:/cisco-open/cisco-8000-kernel-modules.git;branch=main;rev=731909818130375cae729987a9ae44dfa13bfdcb;protocol=https \
"

S = "${WORKDIR}"
MODULES_MODULE_SYMVERS_LOCATION = "git/drivers"

KERNEL_MODULE_AUTOLOAD += " \
    libcisco \
    cisco-fpga-bmc \
    cisco-fpga-info \
    cisco-fpga-msd \
    cisco-fpga-pseq \
    cisco-fpga-i2c-ext \
    cisco-fpga-xil \
"
