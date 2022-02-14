# Copyright 2021-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

DEPENDS += "libbic libfby35-common"
RDEPENDS:${PN} += "libbic libfby35-common"

LDFLAGS += "-lbic -lfby35_common"
