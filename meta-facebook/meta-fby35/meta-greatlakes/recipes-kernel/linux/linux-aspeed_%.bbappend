# Copyright 2020-present Facebook. All Rights Reserved.

LINUX_VERSION_EXTENSION = "-greatlakes"

COMPATIBLE_MACHINE = "greatlakes"

FILESEXTRAPATHS:prepend := "${THISDIR}/plat_conf:"
SRC_URI += "file://greatlakes.cfg"
