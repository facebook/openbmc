# Copyright 2021-present Facebook. All Rights Reserved.
SUMMARY = "One Time Programming Utility"
DESCRIPTION = "Util to read and write OTP memory"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"

LIC_FILES_CHKSUM = "file://otp.c;beginline=4;endline=16;md5=b66b777f082370423b0fa6f12a3dc4db"

inherit meson pkgconfig
LOCAL_URI = " \
    file://aspeed-otp.h \
    file://meson.build \
    file://otp.c \
    file://sha256.c \
    file://sha256.h \
    file://otp_info.h \
    "

FILES:${PN} = "${bindir}"
