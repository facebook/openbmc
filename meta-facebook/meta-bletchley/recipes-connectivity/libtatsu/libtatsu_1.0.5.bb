SUMMARY = "Library handling the communication with Apple's Tatsu Signing Server (TSS)"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "\
    file://COPYING;md5=6ab17b41640564434dda85c06b7124f7 \
"

HOMEPAGE = "http://www.libimobiledevice.org/"

DEPENDS = "libplist curl"
SRCREV = "42329cb756682535c7c0f087987b78d1dd5b16c8"
SRC_URI = "git://github.com/libimobiledevice/libtatsu;protocol=https;branch=master"

inherit autotools pkgconfig
