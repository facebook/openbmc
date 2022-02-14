# Copyright 2020-present Facebook. All Rights Reserved.

LINUX_VERSION_EXTENSION = "-fby35"

COMPATIBLE_MACHINE = "fby35"

FILESEXTRAPATHS:prepend := "${THISDIR}/plat_conf:"
SRC_URI += "file://fby35.cfg"
