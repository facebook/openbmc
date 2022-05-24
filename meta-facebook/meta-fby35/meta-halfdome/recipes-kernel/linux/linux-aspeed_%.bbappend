# Copyright 2020-present Facebook. All Rights Reserved.

LINUX_VERSION_EXTENSION = "-halfdome"

COMPATIBLE_MACHINE = "halfdome"

FILESEXTRAPATHS:prepend := "${THISDIR}/plat_conf:"
SRC_URI += "file://fby35.cfg"
