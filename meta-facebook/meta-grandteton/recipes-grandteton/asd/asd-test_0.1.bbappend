# Copyright 2022-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

CFLAGS:prepend = "-DEXT_JTAG_HNDLR"
