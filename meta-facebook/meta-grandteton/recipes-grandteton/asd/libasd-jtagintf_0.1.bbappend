# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

LDFLAGS += "-lgpio-ctrl -lpal"
DEPENDS += "libgpio-ctrl libpal"
RDEPENDS:${PN} += "libgpio-ctrl libpal"
