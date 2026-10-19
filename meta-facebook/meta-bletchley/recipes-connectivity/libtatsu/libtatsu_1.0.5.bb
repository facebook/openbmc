SUMMARY = "Library handling the communication with Apple's Tatsu Signing Server (TSS)"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "\
    file://COPYING;md5=6ab17b41640564434dda85c06b7124f7 \
"

HOMEPAGE = "http://www.libimobiledevice.org/"

DEPENDS = "libplist curl"
PV = "1.0.5+git"
SRCREV = "cdbca263fa480b118bd458e32c6eeddafd48e35d"
SRC_URI = "git://github.com/libimobiledevice/libtatsu;protocol=https;branch=master"

inherit autotools pkgconfig
