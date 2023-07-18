# Copyright 2023-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

CFLAGS:prepend = "-DIPMB_JTAG_HNDLR"
