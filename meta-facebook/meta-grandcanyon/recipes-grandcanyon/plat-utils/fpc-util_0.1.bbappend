# Copyright 2020-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:prepend := "${THISDIR}/files/fpc-util:"
DEPENDS += " libkv libfbgc-common"
RDEPENDS:${PN} += " libkv libfbgc-common"
LDFLAGS += " -lkv -lfbgc_common"
