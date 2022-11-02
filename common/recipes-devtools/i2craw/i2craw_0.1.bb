LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://i2craw.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://i2craw.c \
    file://meson.build \
    "

inherit meson pkgconfig

DEPENDS = " libobmc-i2c liblog "
