# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "DBus publisher/subscriber example"
DESCRIPTION = "dbus-publisher publishes signals one_sec_timer, three_sec_timer and five_sec_timer every 1, 2 and 3 seconds respectively. " \
              "dbus-subscriber subscribes to a signal and prints whenever signal is received"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://DBusPublisher.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

LOCAL_URI = " \
    file://CMakeLists.txt \
    file://DBusPublisher.cpp \
    file://DBusSubscriber.cpp \
    file://org.openbmc.TimerService.conf \
    "

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

inherit cmake

DEPENDS += "glib-2.0"
