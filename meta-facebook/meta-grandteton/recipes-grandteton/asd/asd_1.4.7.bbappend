# Copyright 2021-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libgpio-ctrl"
RDEPENDS:${PN} += "libgpio-ctrl"

LDFLAGS =+ "-lgpio-ctrl"
