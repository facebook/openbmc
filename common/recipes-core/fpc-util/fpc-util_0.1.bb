# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Front Panel Control Utility"
DESCRIPTION = "Util to override the front panel control remotely"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-only"

# The license GPL-2.0 was removed in Hardknott.
# Use GPL-2.0-only instead.
def lic_file_name(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

    return "GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

LIC_FILES_CHKSUM = "\
    file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)} \
    "

LOCAL_URI = " \
    file://fpc-util.c \
    file://meson.build \
    file://plat/meson.build \
"

inherit meson pkgconfig

DEPENDS += "libpal"
