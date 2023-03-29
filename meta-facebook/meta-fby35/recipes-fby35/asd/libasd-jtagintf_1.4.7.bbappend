# Copyright 2022-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libbic libfby35-common"

LDFLAGS =+ "-lbic -lfby35_common"
CFLAGS:prepend = "-DIPMB_JTAG_HNDLR"
