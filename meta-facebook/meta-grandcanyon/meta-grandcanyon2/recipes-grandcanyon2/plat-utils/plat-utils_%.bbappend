# Copyright 2020-present Facebook. All Rights Reserved.
#
# GrandCanyon 2.0 Project

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://setup-ntp.sh \
    file://sync_date.sh \
    "

do_install:append() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${UNPACKDIR}/setup-ntp.sh \
        ${D}${sysconfdir}/init.d/setup-ntp.sh

    update-rc.d -r ${D} setup-ntp.sh start 19 5 .
}

CFLAGS:prepend = " -DCONFIG_GRANDCANYON2 "
