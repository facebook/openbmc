# Copyright 2021-present Facebook. All Rights Reserved.
SUMMARY = "System Event Gen Utility"
DESCRIPTION = "Utility for creating and injecting system events"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://syseventgen-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI =  "file://Makefile \
            file://syseventgen-util.cpp \
            file://syseventgen-util.h \
            "

S = "${WORKDIR}"
binfiles = "syseventgen-util"

# add libraries here later
# LDFLAGS += "-lpal"

pkgdir = "syseventgen-util"

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    install -d $dst
    install -d $bin
    install -m 755 syseventgen-util ${dst}/syseventgen-util
    ln -snf ../fbpackages/${pkgdir}/syseventgen-util ${bin}/syseventgen-util
}

# add libraries here later
# DEPENDS += "libpal"
# RDEPENDS_${PN} += "libpal"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/syseventgen-util ${prefix}/local/bin"
