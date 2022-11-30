# Copyright 2022-present Facebook. All Rights Reserved.

LINUX_VERSION_EXTENSION = "-minipack"

COMPATIBLE_MACHINE = "minipack"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI += "file://minipack.cfg \
            "
