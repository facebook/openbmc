# Copyright 2022-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libpal"
RDEPENDS:${PN} += "libpal"

LDFLAGS =+ "-lgpio-ctrl -lpal"
