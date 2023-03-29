# Copyright 2022-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libpal"

LDFLAGS =+ "-lgpio-ctrl -lpal"
