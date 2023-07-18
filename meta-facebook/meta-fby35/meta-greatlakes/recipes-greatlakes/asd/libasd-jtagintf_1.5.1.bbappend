# Copyright 2022-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

CFLAGS:prepend = "-DIPMB_JTAG_HNDLR"

DEPENDS += "libbic libfby35-common"
RDEPENDS:${PN} += "libbic libfby35-common"

LDFLAGS =+ "-lbic -lfby35_common"
