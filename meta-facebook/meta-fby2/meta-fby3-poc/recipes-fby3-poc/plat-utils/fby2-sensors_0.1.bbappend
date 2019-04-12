# Copyright 2015-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_prepend := "${THISDIR}/files/:"

SRC_URI  += "file://fby2-sensors \
            "

CFLAGS_prepend = " -DCONFIG_FBY3_POC "
