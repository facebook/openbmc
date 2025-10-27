# Copyright 2020-present Facebook. All Rights Reserved.
#
# GrandCanyon 2.0 Project
LINUX_VERSION_EXTENSION = "-grandcanyon2"

COMPATIBLE_MACHINE = "grandcanyon2"

FILESEXTRAPATHS:prepend := "${THISDIR}/plat_conf:"
SRC_URI += "file://grandcanyon2.cfg"