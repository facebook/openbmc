SUMMARY = "Sensor Reading Tolerance Library"
DESCRIPTION = "Library for Sensor Reading Failure Tolerance"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://snr-tolerance.h;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

inherit meson pkgconfig

LOCAL_URI = " \
    file://snr-tolerance.h \
    file://snr-tolerance.cpp \
    file://meson.build \
    "
