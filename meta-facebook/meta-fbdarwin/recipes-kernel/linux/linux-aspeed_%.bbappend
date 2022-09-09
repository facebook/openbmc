# Copyright 2022-present Facebook. All Rights Reserved.

LINUX_VERSION_EXTENSION = "-fbdarwin"

COMPATIBLE_MACHINE = "fbdarwin"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI += "file://fbdarwin.cfg \
           "
