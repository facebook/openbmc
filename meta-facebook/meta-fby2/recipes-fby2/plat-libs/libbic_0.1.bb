# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)}"

inherit meson pkgconfig

def lic_file_name(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"
    return "GPL-2.0-or-later;md5=fed54355545ffd980b814dab4a3b312c"

SRC_URI = "file://bic"
S = "${WORKDIR}/bic"

#LDFLAGS += " -lmisc-utils -lobmc-i2c -lgpio-ctrl -lcrypto -lfby2_common"

DEPENDS += " \
    libmisc-utils \
    libfby2-common \
    libipmi \
    libipmb \
    libkv \
    plat-utils \
    libobmc-i2c \
    libgpio-ctrl \
    openssl \
"
RDEPENDS:${PN} += "libmisc-utils libfby2-common"
