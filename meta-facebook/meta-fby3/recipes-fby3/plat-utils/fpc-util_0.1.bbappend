# Copyright 2015-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:prepend := "${THISDIR}/files/fpc-util:"
DEPENDS += "libkv libbic libfby3-common"
RDEPENDS:${PN} += "libbic libfby3-common"
