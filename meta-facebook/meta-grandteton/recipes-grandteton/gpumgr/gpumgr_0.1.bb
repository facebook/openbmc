# Copyright 2022-present Meta Platforms Inc. All Rights Reserved.

SUMMARY = "GPU Manager"
DESCRIPTION = "Daemon to monitor and control the accelerator board"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

S="${WORKDIR}/sources"
UNPACKDIR="${S}"

LOCAL_URI = " \
    file://meson.build \
    file://main.cpp \
    "

DEPENDS += "libhgx cli11"

do_install:append() {
    localbindir="${D}/usr/local/bin" 
    install -d ${localbindir}
    ln -s gpumgr ${localbindir}/hgxmgr
}

FILES:${PN} = "${prefix}/local/bin"
